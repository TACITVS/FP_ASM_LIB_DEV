/*
 * fpasm_d3d11_smoke.c — Direct3D 11 end-to-end check of the FP-ASM renderer
 * contract. Not part of the library (which never includes or links D3D);
 * this is a consumer, the way a game's renderer would use it.
 *
 * Headless: renders into a 64x64 offscreen target on the WARP software
 * rasterizer (or the GPU with --hardware), reads the pixels back and checks
 * them. Exit code 0 = every check passed.
 *
 *   red   triangle  CPU path: fp_mat4_mul_vec3_batch computes clip-space
 *                   positions, fp_stream_copy writes them into a mapped
 *                   D3D11_USAGE_DYNAMIC vertex buffer; the vertex shader is
 *                   a pass-through.
 *   green triangle  GPU path: model-space vertices, the MVP matrix uploaded
 *                   with fp_mat4_upload_gfx into a constant buffer, and the
 *                   shader does mul(M, v) (fpasm.hlsli convention).
 *   blue  backdrop  drawn LAST and FARTHER away; with reversed-Z
 *                   (clear 0, GREATER) it must lose the depth test behind
 *                   the triangles and fill the rest of the frame.
 * The expected pixel positions are computed with the library's own
 * projection, so conventions, layout and depth state are all cross-checked.
 *
 * Build (MSYS2 / MinGW):  make TARGET_OS=windows example-d3d11
 */
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <stdio.h>
#include <string.h>

#include "fp_core.h"
#include "fp_dispatch.h"
#include "fp_gfx.h"

#define W 64
#define H 64

static int failures = 0;

#define HR(call) do { HRESULT hr_ = (call); if (FAILED(hr_)) { \
    fprintf(stderr, "FAIL %s -> 0x%08lx (line %d)\n", #call, (unsigned long)hr_, __LINE__); return 2; } } while (0)

static const char shader_src[] =
    "cbuffer Draw : register(b0) { float4x4 mvp; float4 color; };\n"
    "float4 vs_pass(float4 p : POSITION) : SV_Position { return p; }\n"
    "float4 vs_mvp (float4 p : POSITION) : SV_Position { return mul(mvp, p); }\n"
    "float4 ps_main(float4 p : SV_Position) : SV_Target { return color; }\n";

typedef struct { float mvp[16]; float color[4]; } DrawCB;   /* 80 bytes */

static ID3DBlob* compile(const char* entry, const char* target) {
    ID3DBlob *blob = NULL, *err = NULL;
    HRESULT hr = D3DCompile(shader_src, sizeof shader_src - 1, "fpasm_smoke", NULL, NULL,
                            entry, target, D3DCOMPILE_ENABLE_STRICTNESS, 0, &blob, &err);
    if (FAILED(hr)) {
        fprintf(stderr, "shader %s: %s\n", entry, err ? (const char*)ID3D10Blob_GetBufferPointer(err) : "?");
        if (err) ID3D10Blob_Release(err);
        return NULL;
    }
    if (err) ID3D10Blob_Release(err);
    return blob;
}

/* Library-side projection of a world point to a pixel (same matrices). */
static void to_pixel(const Mat4* vp, float x, float y, float z, int* px, int* py) {
    float c[4];
    int r;
    for (r = 0; r < 4; r++) c[r] = vp->m[r] * x + vp->m[4 + r] * y + vp->m[8 + r] * z + vp->m[12 + r];
    *px = (int)((c[0] / c[3] * 0.5f + 0.5f) * W);
    *py = (int)((0.5f - c[1] / c[3] * 0.5f) * H);
}

static void expect(const unsigned char* px, int pitch, int x, int y, const char* what,
                   unsigned r, unsigned g, unsigned b) {
    const unsigned char* p = px + y * pitch + x * 4;
    int ok = (p[0] > 200) == (r > 200) && (p[1] > 200) == (g > 200) && (p[2] > 200) == (b > 200);
    printf("%s %-28s pixel (%2d,%2d) = (%3u,%3u,%3u)\n", ok ? "ok  " : "FAIL", what, x, y, p[0], p[1], p[2]);
    if (!ok) failures++;
}

int main(int argc, char** argv) {
    const int hardware = argc > 1 && strcmp(argv[1], "--hardware") == 0;
    ID3D11Device* dev = NULL;
    ID3D11DeviceContext* ctx = NULL;
    D3D_FEATURE_LEVEL fl;
    fp_gfx_conventions cv;
    Mat4 view, proj, vp;

    /* Renderer conventions: Direct3D 11, reversed-Z. Pure values. */
    fp_dispatch_init();
    fp_gfx_conventions_init(&cv, FP_GFX_D3D11, 1);
    fp_mat4_lookat_gfx(&view, 0, 0, 5, 0, 0, 0, 0, 1, 0, &cv);
    fp_mat4_perspective_gfx(&proj, 1.0471976f, (float)W / H, 0.1f, 100.0f, &cv);
    fp_mat4_mul(&vp, &proj, &view);
    printf("conventions: %s, reversed-Z, depth clear %.0f, compare %s; library default: %s\n",
           fp_gfx_preset_name(cv.preset), fp_gfx_depth_clear_value(&cv),
           fp_gfx_depth_compare(&cv) == FP_GFX_COMPARE_GREATER ? "GREATER" : "LESS",
           fp_gfx_preset_name(fp_gfx_default()->preset));

    HR(D3D11CreateDevice(NULL, hardware ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_WARP, NULL, 0,
                         NULL, 0, D3D11_SDK_VERSION, &dev, &fl, &ctx));
    printf("device: %s, feature level 0x%x\n", hardware ? "hardware" : "WARP", (unsigned)fl);

    /* Targets: RGBA8 color, D32 depth, staging copy for readback. */
    ID3D11Texture2D *rt, *ds, *staging;
    ID3D11RenderTargetView* rtv;
    ID3D11DepthStencilView* dsv;
    {
        D3D11_TEXTURE2D_DESC td = { 0 };
        td.Width = W; td.Height = H; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_RENDER_TARGET;
        HR(ID3D11Device_CreateTexture2D(dev, &td, NULL, &rt));
        HR(ID3D11Device_CreateRenderTargetView(dev, (ID3D11Resource*)rt, NULL, &rtv));
        td.Usage = D3D11_USAGE_STAGING; td.BindFlags = 0; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        HR(ID3D11Device_CreateTexture2D(dev, &td, NULL, &staging));
        td.Format = DXGI_FORMAT_D32_FLOAT; td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_DEPTH_STENCIL; td.CPUAccessFlags = 0;
        HR(ID3D11Device_CreateTexture2D(dev, &td, NULL, &ds));
        HR(ID3D11Device_CreateDepthStencilView(dev, (ID3D11Resource*)ds, NULL, &dsv));
    }

    /* Depth state straight from the conventions. */
    ID3D11DepthStencilState* dss;
    ID3D11RasterizerState* rs;
    {
        D3D11_DEPTH_STENCIL_DESC dd = { 0 };
        D3D11_RASTERIZER_DESC rd = { 0 };
        dd.DepthEnable = TRUE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dd.DepthFunc = fp_gfx_depth_compare(&cv) == FP_GFX_COMPARE_GREATER ? D3D11_COMPARISON_GREATER
                                                                           : D3D11_COMPARISON_LESS;
        HR(ID3D11Device_CreateDepthStencilState(dev, &dd, &dss));
        rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
        HR(ID3D11Device_CreateRasterizerState(dev, &rd, &rs));
    }

    /* Shaders + input layout: one float4 POSITION, 16-byte stride (Vec3f). */
    ID3D11VertexShader *vs_pass, *vs_mvp;
    ID3D11PixelShader* ps;
    ID3D11InputLayout* layout;
    {
        ID3DBlob *b_pass = compile("vs_pass", "vs_4_0"), *b_mvp = compile("vs_mvp", "vs_4_0"),
                 *b_ps = compile("ps_main", "ps_4_0");
        D3D11_INPUT_ELEMENT_DESC el = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
                                        D3D11_INPUT_PER_VERTEX_DATA, 0 };
        if (!b_pass || !b_mvp || !b_ps) return 2;
        HR(ID3D11Device_CreateVertexShader(dev, ID3D10Blob_GetBufferPointer(b_pass), ID3D10Blob_GetBufferSize(b_pass), NULL, &vs_pass));
        HR(ID3D11Device_CreateVertexShader(dev, ID3D10Blob_GetBufferPointer(b_mvp), ID3D10Blob_GetBufferSize(b_mvp), NULL, &vs_mvp));
        HR(ID3D11Device_CreatePixelShader(dev, ID3D10Blob_GetBufferPointer(b_ps), ID3D10Blob_GetBufferSize(b_ps), NULL, &ps));
        HR(ID3D11Device_CreateInputLayout(dev, &el, 1, ID3D10Blob_GetBufferPointer(b_pass), ID3D10Blob_GetBufferSize(b_pass), &layout));
        ID3D10Blob_Release(b_pass); ID3D10Blob_Release(b_mvp); ID3D10Blob_Release(b_ps);
    }

    /* Buffers: a dynamic vertex buffer (rewritten each draw) and a dynamic
     * constant buffer sized with fp_gfx_cbuffer_size. */
    ID3D11Buffer *vb, *cb;
    {
        D3D11_BUFFER_DESC bd = { 0 };
        bd.ByteWidth = 3 * FP_GFX_VEC3F_STRIDE;
        bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HR(ID3D11Device_CreateBuffer(dev, &bd, NULL, &vb));
        bd.ByteWidth = (UINT)fp_gfx_cbuffer_size(sizeof(DrawCB));
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        HR(ID3D11Device_CreateBuffer(dev, &bd, NULL, &cb));
    }

    {
        const float clear[4] = { 0, 0, 0, 1 };
        const UINT stride = FP_GFX_VEC3F_STRIDE, offset = 0;
        D3D11_VIEWPORT vpd = { 0, 0, W, H, 0, 1 };
        ID3D11DeviceContext_OMSetRenderTargets(ctx, 1, &rtv, dsv);
        ID3D11DeviceContext_OMSetDepthStencilState(ctx, dss, 0);
        ID3D11DeviceContext_RSSetState(ctx, rs);
        ID3D11DeviceContext_RSSetViewports(ctx, 1, &vpd);
        ID3D11DeviceContext_ClearRenderTargetView(ctx, rtv, clear);
        ID3D11DeviceContext_ClearDepthStencilView(ctx, dsv, D3D11_CLEAR_DEPTH, fp_gfx_depth_clear_value(&cv), 0);
        ID3D11DeviceContext_IASetInputLayout(ctx, layout);
        ID3D11DeviceContext_IASetPrimitiveTopology(ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D11DeviceContext_IASetVertexBuffers(ctx, 0, 1, &vb, &stride, &offset);
        ID3D11DeviceContext_VSSetConstantBuffers(ctx, 0, 1, &cb);
        ID3D11DeviceContext_PSSetConstantBuffers(ctx, 0, 1, &cb);
        ID3D11DeviceContext_PSSetShader(ctx, ps, NULL, 0);
    }

    static const Vec3f tri_red[3]   = { { -2.0f, -0.5f, 0, 1 }, { -0.5f, -0.5f, 0, 1 }, { -1.25f, 0.8f, 0, 1 } };
    static const Vec3f tri_green[3] = { {  0.5f, -0.5f, 0, 1 }, {  2.0f, -0.5f, 0, 1 }, {  1.25f, 0.8f, 0, 1 } };
    static const Vec3f backdrop[3]  = { { -40, -40, -2, 1 }, { 40, -40, -2, 1 }, { 0, 40, -2, 1 } };
    const struct { const Vec3f* v; int cpu; float color[4]; } draws[3] = {
        { tri_red,   1, { 1, 0, 0, 1 } },
        { tri_green, 0, { 0, 1, 0, 1 } },
        { backdrop,  1, { 0, 0, 1, 1 } },
    };
    int d;
    for (d = 0; d < 3; d++) {
        D3D11_MAPPED_SUBRESOURCE m;
        DrawCB dc;
        Vec3f clip[3];
        /* constant buffer: matrix in the shader's layout + color */
        fp_mat4_upload_gfx(dc.mvp, &vp, &cv);
        memcpy(dc.color, draws[d].color, sizeof dc.color);
        HR(ID3D11DeviceContext_Map(ctx, (ID3D11Resource*)cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m));
        fp_stream_copy(m.pData, &dc, sizeof dc);
        ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource*)cb, 0);
        /* vertices: CPU path transforms with the library, GPU path uploads model space */
        if (draws[d].cpu) fp_mat4_mul_vec3_batch(clip, &vp, draws[d].v, 3);
        HR(ID3D11DeviceContext_Map(ctx, (ID3D11Resource*)vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m));
        fp_stream_copy(m.pData, draws[d].cpu ? clip : draws[d].v, 3 * sizeof(Vec3f));
        ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource*)vb, 0);
        ID3D11DeviceContext_VSSetShader(ctx, draws[d].cpu ? vs_pass : vs_mvp, NULL, 0);
        ID3D11DeviceContext_Draw(ctx, 3, 0);
    }

    /* Read back and check against the library's own projection. */
    {
        D3D11_MAPPED_SUBRESOURCE m;
        int x, y;
        ID3D11DeviceContext_CopyResource(ctx, (ID3D11Resource*)staging, (ID3D11Resource*)rt);
        HR(ID3D11DeviceContext_Map(ctx, (ID3D11Resource*)staging, 0, D3D11_MAP_READ, 0, &m));
        to_pixel(&vp, -1.25f, -0.067f, 0, &x, &y);
        expect(m.pData, (int)m.RowPitch, x, y, "red (CPU transform+stream)", 255, 0, 0);
        to_pixel(&vp, 1.25f, -0.067f, 0, &x, &y);
        expect(m.pData, (int)m.RowPitch, x, y, "green (cbuffer mul(M,v))", 0, 255, 0);
        expect(m.pData, (int)m.RowPitch, 1, 1, "blue backdrop (corner)", 0, 0, 255);
        expect(m.pData, (int)m.RowPitch, W / 2, H / 2, "blue between triangles", 0, 0, 255);
        ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource*)staging, 0);
    }

    printf("\n%s (%d failures)\n", failures ? "SOME FAILED" : "ALL PASS", failures);
    return failures != 0;
}
