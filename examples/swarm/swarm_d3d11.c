/*
 * swarm_d3d11.c — "Verdant Swarm": the FP-ASM galaxy demo on Direct3D 11.
 *
 * A spiral galaxy of up to millions of stars, simulated on the CPU every
 * frame (swarm_sim.c) and drawn as instanced additive sprites into an HDR
 * target, then tonemapped. The projection stage writes clip-space positions
 * straight into a mapped D3D11_USAGE_DYNAMIC vertex buffer, from all worker
 * threads: no staging copy. The HUD shows what every simulation stage costs
 * with FP-ASM kernels and with plain C (every 3rd frame runs both, back to
 * back on the same data), so the speedup is measured, live, on your CPU.
 *
 *   verdant_swarm [--stars N] [--threads N] [--mode fpasm|c] [--warp|--hardware]
 *                 [--size WxH] [--vsync]
 *                 [--headless] [--frames N] [--capture out.bmp]
 *
 * Keys: C switch FP-ASM / plain C   T thread count   K kernel tier cap
 *       Space pause   H hide HUD    Esc quit
 *
 * --headless renders offscreen (WARP unless --hardware), runs --frames N at
 * a fixed 60 Hz timestep, prints the timing table and, with --capture,
 * writes the last frame as a BMP. Exit code 0 = the image is not blank.
 *
 * The library knows nothing about Direct3D: it only provides the matrices
 * (fp_gfx conventions: D3D11, reversed-Z, infinite far plane) and kernels.
 *
 * Build (MSYS2 / MinGW, or cross):  make TARGET_OS=windows demo-swarm
 */
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swarm_sim.h"
#include "fp_core.h"
#include "fp_cpu.h"
#include "fp_dispatch.h"
#include "fp_gfx.h"

#define HR(call) do { HRESULT hr_ = (call); if (FAILED(hr_)) { \
    fprintf(stderr, "FAIL %s -> 0x%08lx (line %d)\n", #call, (unsigned long)hr_, __LINE__); exit(2); } } while (0)
#define RELEASE(p) do { if (p) { IUnknown_Release((IUnknown*)(p)); (p) = NULL; } } while (0)

/* ------------------------------------------------------------- shaders */
static const char shader_src[] =
    "cbuffer Frame : register(b0) { float2 proj_scale; float2 min_ndc; float2 screen; float exposure; float pad; };\n"
    "Texture2D hdr  : register(t0);\n"
    "Texture2D font : register(t1);\n"
    "SamplerState pt : register(s0);\n"
    /* stars: one instance per star, 4 strip vertices generated from SV_VertexID */
    "struct StarIn { float4 clip : CLIP; float4 col : COLOR; };\n"
    "struct StarV  { float4 pos : SV_Position; float2 uv : TEXCOORD0; float3 col : COLOR; };\n"
    "StarV vs_star(StarIn s, uint vid : SV_VertexID) {\n"
    "  StarV o;\n"
    "  float2 c = float2((vid & 1) ? 1.0 : -1.0, (vid & 2) ? 1.0 : -1.0);\n"
    "  float2 world = s.col.w * proj_scale;\n"            /* world-size sprite, in clip units */
    "  float2 floor_ = min_ndc * s.clip.w;\n"             /* ...but never under ~1.5 px     */
    "  float k = saturate(world.x / max(floor_.x, 1e-6));\n"
    "  o.pos = s.clip;\n"
    "  o.pos.xy += c * max(world, floor_);\n"
    "  o.uv = c;\n"
    "  o.col = s.col.rgb * (k * k);\n"                    /* keep energy when enlarged */
    "  return o;\n"
    "}\n"
    "float4 ps_star(StarV i) : SV_Target {\n"
    "  float a = max(exp(-4.0 * dot(i.uv, i.uv)) - 0.0183, 0.0);\n"
    "  return float4(i.col * a * exposure, 1.0);\n"
    "}\n"
    /* fullscreen tonemap */
    "float4 vs_full(uint vid : SV_VertexID) : SV_Position {\n"
    "  float2 p = float2((vid << 1) & 2, vid & 2);\n"
    "  return float4(p * float2(2, -2) + float2(-1, 1), 0, 1);\n"
    "}\n"
    "float4 ps_tone(float4 p : SV_Position) : SV_Target {\n"
    "  float3 c = hdr.Load(int3(p.xy, 0)).rgb;\n"
    "  float2 q = p.xy / screen - 0.5;\n"
    "  c += float3(0.004, 0.006, 0.014) * (1.0 - dot(q, q));\n" /* deep-space backdrop */
    "  c = 1.0 - exp(-c);\n"
    "  return float4(pow(c, 1.0 / 2.2), 1.0);\n"
    "}\n"
    /* HUD: pixel-space quads, glyph coverage from the font atlas */
    "struct HudIn { float4 pu : POS; float4 col : COLOR; };\n"
    "struct HudV  { float4 pos : SV_Position; float2 uv : TEXCOORD0; float4 col : COLOR; };\n"
    "HudV vs_hud(HudIn v) {\n"
    "  HudV o;\n"
    "  o.pos = float4(v.pu.x / screen.x * 2 - 1, 1 - v.pu.y / screen.y * 2, 0, 1);\n"
    "  o.uv = v.pu.zw; o.col = v.col;\n"
    "  return o;\n"
    "}\n"
    "float4 ps_hud(HudV i) : SV_Target { return float4(i.col.rgb, i.col.a * font.Sample(pt, i.uv).r); }\n";

typedef struct { float proj_scale[2], min_ndc[2], screen[2], exposure, pad; } FrameCB;

static ID3DBlob* compile(const char* entry, const char* target) {
    ID3DBlob *blob = NULL, *err = NULL;
    HRESULT hr = D3DCompile(shader_src, sizeof shader_src - 1, "verdant_swarm", NULL, NULL,
                            entry, target, D3DCOMPILE_ENABLE_STRICTNESS, 0, &blob, &err);
    if (FAILED(hr)) {
        fprintf(stderr, "shader %s: %s\n", entry, err ? (const char*)ID3D10Blob_GetBufferPointer(err) : "?");
        exit(2);
    }
    RELEASE(err);
    return blob;
}

/* ------------------------------------------------------------ options */
typedef struct {
    size_t stars;
    int threads, mode, hardware, warp, width, height, vsync, headless, frames;
    const char* capture;
} options;

static void usage(void) {
    printf("usage: verdant_swarm [--stars N] [--threads N] [--mode fpasm|c] [--warp|--hardware]\n"
           "                     [--size WxH] [--vsync] [--headless] [--frames N] [--capture out.bmp]\n"
           "keys:  C mode   T threads   K kernel tier   Space pause   H HUD   Esc quit\n");
}

static int parse(options* o, int argc, char** argv) {
    int i;
    memset(o, 0, sizeof *o);
    o->stars = 1u << 20; o->width = 1280; o->height = 720;
    for (i = 1; i < argc; i++) {
        const char* a = argv[i];
        const char* v = i + 1 < argc ? argv[i + 1] : NULL;
        if (!strcmp(a, "--stars") && v) { o->stars = (size_t)strtoull(v, NULL, 10); i++; }
        else if (!strcmp(a, "--threads") && v) { o->threads = atoi(v); i++; }
        else if (!strcmp(a, "--mode") && v) { o->mode = !strcmp(v, "c") || !strcmp(v, "C"); i++; }
        else if (!strcmp(a, "--frames") && v) { o->frames = atoi(v); i++; }
        else if (!strcmp(a, "--capture") && v) { o->capture = v; i++; }
        else if (!strcmp(a, "--size") && v) { sscanf(v, "%dx%d", &o->width, &o->height); i++; }
        else if (!strcmp(a, "--headless")) o->headless = 1;
        else if (!strcmp(a, "--hardware")) o->hardware = 1;
        else if (!strcmp(a, "--warp")) o->warp = 1;
        else if (!strcmp(a, "--vsync")) o->vsync = 1;
        else { usage(); return -1; }
    }
    if (o->stars < 1024) o->stars = 1024;
    if (o->width < 320) o->width = 320;
    if (o->height < 240) o->height = 240;
    if (o->headless && o->frames <= 0) o->frames = 60;
    return 0;
}

/* --------------------------------------------------------------- HUD */
#define ATLAS_COLS 16
#define ATLAS_ROWS 6                          /* ASCII 32..127; 127 = solid cell */
typedef struct { float x, y, u, v, r, g, b, a; } hud_vtx;
#define HUD_MAX_VERTS (6 * 6000)

static struct {
    int cw, ch, aw, ah;                       /* glyph cell and atlas size, px */
    hud_vtx v[HUD_MAX_VERTS];
    int n;
} hud;

static ID3D11ShaderResourceView* make_font_atlas(ID3D11Device* dev) {
    HDC dc = CreateCompatibleDC(NULL);
    HFONT font = CreateFontA(-15, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    BITMAPINFO bi;
    void* bits = NULL;
    HBITMAP bmp;
    SIZE sz;
    TEXTMETRICA tm;
    unsigned char* r8;
    int c, x, y;
    ID3D11Texture2D* tex;
    ID3D11ShaderResourceView* srv;
    D3D11_TEXTURE2D_DESC td = { 0 };
    D3D11_SUBRESOURCE_DATA init;

    SelectObject(dc, font);
    GetTextMetricsA(dc, &tm);
    GetTextExtentPoint32A(dc, "M", 1, &sz);
    hud.cw = sz.cx > 0 ? sz.cx : 8;
    hud.ch = tm.tmHeight > 0 ? tm.tmHeight : 16;
    hud.aw = ATLAS_COLS * hud.cw;
    hud.ah = ATLAS_ROWS * hud.ch;

    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = hud.aw;
    bi.bmiHeader.biHeight = -hud.ah;          /* top-down */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    SelectObject(dc, bmp);
    memset(bits, 0, (size_t)hud.aw * hud.ah * 4);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkMode(dc, TRANSPARENT);
    for (c = 32; c < 127; c++) {
        char ch = (char)c;
        TextOutA(dc, ((c - 32) % ATLAS_COLS) * hud.cw, ((c - 32) / ATLAS_COLS) * hud.ch, &ch, 1);
    }
    GdiFlush();

    r8 = malloc((size_t)hud.aw * hud.ah);
    for (y = 0; y < hud.ah; y++)
        for (x = 0; x < hud.aw; x++) {
            const unsigned char* p = (const unsigned char*)bits + ((size_t)y * hud.aw + x) * 4;
            r8[(size_t)y * hud.aw + x] = p[0] > p[1] ? (p[0] > p[2] ? p[0] : p[2]) : (p[1] > p[2] ? p[1] : p[2]);
        }
    for (y = 0; y < hud.ch; y++)              /* cell 127: solid, for rectangles and bars */
        memset(r8 + (size_t)((127 - 32) / ATLAS_COLS * hud.ch + y) * hud.aw + (127 - 32) % ATLAS_COLS * hud.cw,
               255, (size_t)hud.cw);

    td.Width = (UINT)hud.aw; td.Height = (UINT)hud.ah; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8_UNORM; td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    init.pSysMem = r8; init.SysMemPitch = (UINT)hud.aw; init.SysMemSlicePitch = 0;
    HR(ID3D11Device_CreateTexture2D(dev, &td, &init, &tex));
    HR(ID3D11Device_CreateShaderResourceView(dev, (ID3D11Resource*)tex, NULL, &srv));
    RELEASE(tex);
    free(r8);
    DeleteObject(bmp);
    DeleteObject(font);
    DeleteDC(dc);
    return srv;
}

static void hud_quad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
                     const float col[4]) {
    hud_vtx q[4] = {
        { x0, y0, u0, v0, col[0], col[1], col[2], col[3] }, { x1, y0, u1, v0, col[0], col[1], col[2], col[3] },
        { x0, y1, u0, v1, col[0], col[1], col[2], col[3] }, { x1, y1, u1, v1, col[0], col[1], col[2], col[3] },
    };
    if (hud.n + 6 > HUD_MAX_VERTS) return;
    hud.v[hud.n++] = q[0]; hud.v[hud.n++] = q[1]; hud.v[hud.n++] = q[2];
    hud.v[hud.n++] = q[2]; hud.v[hud.n++] = q[1]; hud.v[hud.n++] = q[3];
}

static void hud_cell_uv(int c, float* u0, float* v0, float* u1, float* v1) {
    int i = c - 32;
    *u0 = (float)(i % ATLAS_COLS * hud.cw) / hud.aw;
    *v0 = (float)(i / ATLAS_COLS * hud.ch) / hud.ah;
    *u1 = *u0 + (float)hud.cw / hud.aw;
    *v1 = *v0 + (float)hud.ch / hud.ah;
}

static void hud_rect(float x, float y, float w, float h, const float col[4]) {
    float u0, v0, u1, v1, du, dv;
    hud_cell_uv(127, &u0, &v0, &u1, &v1);
    du = (u1 - u0) * 0.5f; dv = (v1 - v0) * 0.5f;    /* sample the middle of the solid cell */
    hud_quad(x, y, x + w, y + h, u0 + du, v0 + dv, u0 + du, v0 + dv, col);
}

static float hud_text(float x, float y, const float col[4], const char* fmt, ...) {
    static const float shadow[4] = { 0, 0, 0, 0.85f };
    char buf[256];
    const char* s;
    float cx = x;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    for (s = buf; *s; s++, cx += (float)hud.cw) {
        float u0, v0, u1, v1;
        int c = (unsigned char)*s;
        if (c <= 32 || c >= 127) continue;
        hud_cell_uv(c, &u0, &v0, &u1, &v1);
        hud_quad(cx + 1, y + 1, cx + 1 + hud.cw, y + 1 + hud.ch, u0, v0, u1, v1, shadow);
        hud_quad(cx, y, cx + hud.cw, y + hud.ch, u0, v0, u1, v1, col);
    }
    return cx - x;
}

static const char* thousands(size_t n, char* buf) {
    char tmp[32];
    int len, i, o = 0;
    len = snprintf(tmp, sizeof tmp, "%zu", n);
    for (i = 0; i < len; i++) {
        if (i && (len - i) % 3 == 0) buf[o++] = ',';
        buf[o++] = tmp[i];
    }
    buf[o] = 0;
    return buf;
}

/* --------------------------------------------------------- statistics */
#define WARMUP 5                          /* frames not timed (first touches, caches) */
typedef struct {
    double ms[2][SWARM_STAGES];       /* smoothed, per mode, from the A/B probes */
    int    seen[2];
    double frame_ms;                  /* smoothed wall time per frame */
} perf_t;

static void perf_add(perf_t* p, int mode, const swarm_timing* t) {
    int s;
    for (s = 0; s < SWARM_STAGES; s++)
        p->ms[mode][s] = p->seen[mode] ? p->ms[mode][s] * 0.9 + t->ms[s] * 0.1 : t->ms[s];
    p->seen[mode]++;
}

static double perf_total(const perf_t* p, int mode) {
    double t = 0;
    int s;
    for (s = 0; s < SWARM_STAGES; s++) t += p->ms[mode][s];
    return t;
}

/* ------------------------------------------------------------- window */
static struct { int quit, toggle_mode, next_threads, next_tier, pause, hud; } keys;

static LRESULT CALLBACK wndproc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_KEYDOWN:
        switch (wp) {
        case VK_ESCAPE: keys.quit = 1; break;
        case 'C': keys.toggle_mode = 1; break;
        case 'T': keys.next_threads = 1; break;
        case 'K': keys.next_tier = 1; break;
        case 'H': keys.hud ^= 1; break;
        case VK_SPACE: keys.pause ^= 1; break;
        }
        return 0;
    case WM_CLOSE:
    case WM_DESTROY:
        keys.quit = 1;
        return 0;
    }
    return DefWindowProcA(w, msg, wp, lp);
}

static HWND make_window(int width, int height) {
    WNDCLASSA wc = { 0 };
    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    RECT r = { 0, 0, width, height };
    HWND w;
    wc.lpfnWndProc = wndproc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "VerdantSwarm";
    RegisterClassA(&wc);
    AdjustWindowRect(&r, style, FALSE);
    w = CreateWindowA("VerdantSwarm", "Verdant Swarm - FP-ASM", style, CW_USEDEFAULT, CW_USEDEFAULT,
                      r.right - r.left, r.bottom - r.top, NULL, NULL, wc.hInstance, NULL);
    ShowWindow(w, SW_SHOW);
    return w;
}

/* ------------------------------------------------------------ capture */
static int write_bmp(const char* path, const unsigned char* rgba, int pitch, int w, int h) {
    unsigned char hdr[54] = { 'B', 'M' };
    int row = (w * 3 + 3) & ~3, x, y;
    unsigned size = 54u + (unsigned)row * (unsigned)h;
    unsigned char* line = calloc((size_t)row, 1);
    FILE* f = fopen(path, "wb");
    if (!f || !line) { free(line); if (f) fclose(f); return -1; }
#define PUT32(o, v) do { hdr[o] = (unsigned char)(v); hdr[o + 1] = (unsigned char)((v) >> 8); \
                         hdr[o + 2] = (unsigned char)((v) >> 16); hdr[o + 3] = (unsigned char)((v) >> 24); } while (0)
    PUT32(2, size); PUT32(10, 54u); PUT32(14, 40u); PUT32(18, (unsigned)w); PUT32(22, (unsigned)h);
    hdr[26] = 1; hdr[28] = 24;
    PUT32(34, (unsigned)row * (unsigned)h);
#undef PUT32
    fwrite(hdr, 1, sizeof hdr, f);
    for (y = h - 1; y >= 0; y--) {                    /* BMP rows are bottom-up, BGR */
        const unsigned char* p = rgba + (size_t)y * pitch;
        for (x = 0; x < w; x++) {
            line[3 * x + 0] = p[4 * x + 2];
            line[3 * x + 1] = p[4 * x + 1];
            line[3 * x + 2] = p[4 * x + 0];
        }
        fwrite(line, 1, (size_t)row, f);
    }
    free(line);
    return fclose(f);
}

/* ------------------------------------------------------------- render */
typedef struct {
    ID3D11Device* dev;
    ID3D11DeviceContext* ctx;
    IDXGISwapChain* swap;
    ID3D11Texture2D* target;          /* backbuffer or offscreen RGBA8 */
    ID3D11RenderTargetView* target_rtv;
    ID3D11Texture2D* hdr;
    ID3D11RenderTargetView* hdr_rtv;
    ID3D11ShaderResourceView* hdr_srv;
    ID3D11ShaderResourceView* font_srv;
    ID3D11VertexShader *vs_star, *vs_full, *vs_hud;
    ID3D11PixelShader *ps_star, *ps_tone, *ps_hud;
    ID3D11InputLayout *star_layout, *hud_layout;
    ID3D11Buffer *clip_vb, *color_vb, *hud_vb, *cb;
    ID3D11BlendState *additive, *alpha;
    ID3D11RasterizerState* rs;
    ID3D11DepthStencilState* no_depth;
    ID3D11SamplerState* point;
    ID3D11Query* idle;                /* WARP only: see renderer_wait_idle */
    int width, height;
    const char* driver;
} renderer;

static void make_buffer(ID3D11Device* dev, UINT bytes, D3D11_USAGE usage, UINT bind, const void* init,
                        ID3D11Buffer** out) {
    D3D11_BUFFER_DESC bd = { 0 };
    D3D11_SUBRESOURCE_DATA sd = { 0 };
    bd.ByteWidth = bytes; bd.Usage = usage; bd.BindFlags = bind;
    bd.CPUAccessFlags = usage == D3D11_USAGE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0;
    sd.pSysMem = init;
    HR(ID3D11Device_CreateBuffer(dev, &bd, init ? &sd : NULL, out));
}

static void renderer_init(renderer* r, const options* o, HWND wnd, const swarm_galaxy* g) {
    D3D_DRIVER_TYPE want = (o->warp || (o->headless && !o->hardware)) ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE;
    D3D_FEATURE_LEVEL fl;
    HRESULT hr;
    memset(r, 0, sizeof *r);
    r->width = o->width; r->height = o->height;

    if (wnd) {
        DXGI_SWAP_CHAIN_DESC sd = { 0 };
        sd.BufferDesc.Width = (UINT)o->width; sd.BufferDesc.Height = (UINT)o->height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;
        sd.OutputWindow = wnd;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        hr = D3D11CreateDeviceAndSwapChain(NULL, want, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd,
                                           &r->swap, &r->dev, &fl, &r->ctx);
        if (FAILED(hr) && want == D3D_DRIVER_TYPE_HARDWARE) {
            fprintf(stderr, "no hardware D3D11 device (0x%08lx), falling back to WARP\n", (unsigned long)hr);
            want = D3D_DRIVER_TYPE_WARP;
            hr = D3D11CreateDeviceAndSwapChain(NULL, want, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd,
                                               &r->swap, &r->dev, &fl, &r->ctx);
        }
        HR(hr);
        HR(IDXGISwapChain_GetBuffer(r->swap, 0, &IID_ID3D11Texture2D, (void**)&r->target));
    } else {
        D3D11_TEXTURE2D_DESC td = { 0 };
        HR(D3D11CreateDevice(NULL, want, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &r->dev, &fl, &r->ctx));
        td.Width = (UINT)o->width; td.Height = (UINT)o->height; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_RENDER_TARGET;
        HR(ID3D11Device_CreateTexture2D(r->dev, &td, NULL, &r->target));
    }
    r->driver = want == D3D_DRIVER_TYPE_WARP ? "WARP (software)" : "hardware";
    HR(ID3D11Device_CreateRenderTargetView(r->dev, (ID3D11Resource*)r->target, NULL, &r->target_rtv));

    {   /* HDR accumulation target */
        D3D11_TEXTURE2D_DESC td = { 0 };
        td.Width = (UINT)o->width; td.Height = (UINT)o->height; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        HR(ID3D11Device_CreateTexture2D(r->dev, &td, NULL, &r->hdr));
        HR(ID3D11Device_CreateRenderTargetView(r->dev, (ID3D11Resource*)r->hdr, NULL, &r->hdr_rtv));
        HR(ID3D11Device_CreateShaderResourceView(r->dev, (ID3D11Resource*)r->hdr, NULL, &r->hdr_srv));
    }

    {   /* shaders and input layouts */
        ID3DBlob *b_vs_star = compile("vs_star", "vs_4_0"), *b_ps_star = compile("ps_star", "ps_4_0");
        ID3DBlob *b_vs_full = compile("vs_full", "vs_4_0"), *b_ps_tone = compile("ps_tone", "ps_4_0");
        ID3DBlob *b_vs_hud = compile("vs_hud", "vs_4_0"), *b_ps_hud = compile("ps_hud", "ps_4_0");
        /* per-INSTANCE data only: slot 0 = clip position (rewritten every
         * frame by the simulation), slot 1 = static colour + size */
        const D3D11_INPUT_ELEMENT_DESC star_el[2] = {
            { "CLIP",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };
        const D3D11_INPUT_ELEMENT_DESC hud_el[2] = {
            { "POS",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        HR(ID3D11Device_CreateVertexShader(r->dev, ID3D10Blob_GetBufferPointer(b_vs_star), ID3D10Blob_GetBufferSize(b_vs_star), NULL, &r->vs_star));
        HR(ID3D11Device_CreatePixelShader(r->dev, ID3D10Blob_GetBufferPointer(b_ps_star), ID3D10Blob_GetBufferSize(b_ps_star), NULL, &r->ps_star));
        HR(ID3D11Device_CreateVertexShader(r->dev, ID3D10Blob_GetBufferPointer(b_vs_full), ID3D10Blob_GetBufferSize(b_vs_full), NULL, &r->vs_full));
        HR(ID3D11Device_CreatePixelShader(r->dev, ID3D10Blob_GetBufferPointer(b_ps_tone), ID3D10Blob_GetBufferSize(b_ps_tone), NULL, &r->ps_tone));
        HR(ID3D11Device_CreateVertexShader(r->dev, ID3D10Blob_GetBufferPointer(b_vs_hud), ID3D10Blob_GetBufferSize(b_vs_hud), NULL, &r->vs_hud));
        HR(ID3D11Device_CreatePixelShader(r->dev, ID3D10Blob_GetBufferPointer(b_ps_hud), ID3D10Blob_GetBufferSize(b_ps_hud), NULL, &r->ps_hud));
        HR(ID3D11Device_CreateInputLayout(r->dev, star_el, 2, ID3D10Blob_GetBufferPointer(b_vs_star), ID3D10Blob_GetBufferSize(b_vs_star), &r->star_layout));
        HR(ID3D11Device_CreateInputLayout(r->dev, hud_el, 2, ID3D10Blob_GetBufferPointer(b_vs_hud), ID3D10Blob_GetBufferSize(b_vs_hud), &r->hud_layout));
        RELEASE(b_vs_star); RELEASE(b_ps_star); RELEASE(b_vs_full);
        RELEASE(b_ps_tone); RELEASE(b_vs_hud); RELEASE(b_ps_hud);
    }

    /* buffers: the clip-position stream is DYNAMIC (the simulation maps it and
     * writes into it directly); colours never change, so they are IMMUTABLE */
    make_buffer(r->dev, (UINT)(g->n * FP_GFX_VEC3F_STRIDE), D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, NULL, &r->clip_vb);
    make_buffer(r->dev, (UINT)(g->n * 4 * sizeof(float)), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, g->color, &r->color_vb);
    make_buffer(r->dev, (UINT)sizeof hud.v, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, NULL, &r->hud_vb);
    make_buffer(r->dev, (UINT)fp_gfx_cbuffer_size(sizeof(FrameCB)), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, NULL, &r->cb);

    {   /* fixed-function state */
        D3D11_BLEND_DESC bd = { 0 };
        D3D11_RASTERIZER_DESC rd = { 0 };
        D3D11_DEPTH_STENCIL_DESC dd = { 0 };
        D3D11_SAMPLER_DESC sd = { 0 };
        bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        HR(ID3D11Device_CreateBlendState(r->dev, &bd, &r->additive));
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        HR(ID3D11Device_CreateBlendState(r->dev, &bd, &r->alpha));
        rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
        HR(ID3D11Device_CreateRasterizerState(r->dev, &rd, &r->rs));
        dd.DepthEnable = FALSE;                         /* additive light: order-independent */
        HR(ID3D11Device_CreateDepthStencilState(r->dev, &dd, &r->no_depth));
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        HR(ID3D11Device_CreateSamplerState(r->dev, &sd, &r->point));
    }
    r->font_srv = make_font_atlas(r->dev);
    if (want == D3D_DRIVER_TYPE_WARP) {
        D3D11_QUERY_DESC qd = { D3D11_QUERY_EVENT, 0 };
        HR(ID3D11Device_CreateQuery(r->dev, &qd, &r->idle));
    }
}

/* WARP rasterizes on the CPU, with its own worker threads, while the next
 * frame is being simulated: the simulation timings would then measure the
 * fight for cores. On WARP, wait for the rasterizer to go idle first. */
static void renderer_wait_idle(renderer* r) {
    if (!r->idle) return;
    ID3D11DeviceContext_End(r->ctx, (ID3D11Asynchronous*)r->idle);
    while (ID3D11DeviceContext_GetData(r->ctx, (ID3D11Asynchronous*)r->idle, NULL, 0, 0) == S_FALSE)
        Sleep(0);
}

static void renderer_draw(renderer* r, size_t stars, const FrameCB* fcb, int show_hud) {
    ID3D11DeviceContext* c = r->ctx;
    D3D11_MAPPED_SUBRESOURCE m;
    const float clear[4] = { 0, 0, 0, 0 }, blend[4] = { 0, 0, 0, 0 };
    D3D11_VIEWPORT vp = { 0, 0, (float)r->width, (float)r->height, 0, 1 };
    ID3D11ShaderResourceView* none = NULL;
    ID3D11Buffer* vbs[2] = { r->clip_vb, r->color_vb };
    UINT strides[2] = { FP_GFX_VEC3F_STRIDE, 4 * sizeof(float) }, offsets[2] = { 0, 0 };
    UINT hud_stride = sizeof(hud_vtx), zero = 0;

    HR(ID3D11DeviceContext_Map(c, (ID3D11Resource*)r->cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m));
    memcpy(m.pData, fcb, sizeof *fcb);
    ID3D11DeviceContext_Unmap(c, (ID3D11Resource*)r->cb, 0);

    ID3D11DeviceContext_RSSetViewports(c, 1, &vp);
    ID3D11DeviceContext_RSSetState(c, r->rs);
    ID3D11DeviceContext_OMSetDepthStencilState(c, r->no_depth, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(c, 0, 1, &r->cb);
    ID3D11DeviceContext_PSSetConstantBuffers(c, 0, 1, &r->cb);
    ID3D11DeviceContext_PSSetSamplers(c, 0, 1, &r->point);

    /* 1. stars -> HDR, additive */
    ID3D11DeviceContext_OMSetRenderTargets(c, 1, &r->hdr_rtv, NULL);
    ID3D11DeviceContext_ClearRenderTargetView(c, r->hdr_rtv, clear);
    ID3D11DeviceContext_OMSetBlendState(c, r->additive, blend, 0xffffffff);
    ID3D11DeviceContext_IASetInputLayout(c, r->star_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(c, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ID3D11DeviceContext_IASetVertexBuffers(c, 0, 2, vbs, strides, offsets);
    ID3D11DeviceContext_VSSetShader(c, r->vs_star, NULL, 0);
    ID3D11DeviceContext_PSSetShader(c, r->ps_star, NULL, 0);
    ID3D11DeviceContext_DrawInstanced(c, 4, (UINT)stars, 0, 0);

    /* 2. tonemap -> target */
    ID3D11DeviceContext_OMSetRenderTargets(c, 1, &r->target_rtv, NULL);
    ID3D11DeviceContext_OMSetBlendState(c, NULL, blend, 0xffffffff);
    ID3D11DeviceContext_IASetInputLayout(c, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(c, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(c, r->vs_full, NULL, 0);
    ID3D11DeviceContext_PSSetShader(c, r->ps_tone, NULL, 0);
    ID3D11DeviceContext_PSSetShaderResources(c, 0, 1, &r->hdr_srv);
    ID3D11DeviceContext_Draw(c, 3, 0);
    ID3D11DeviceContext_PSSetShaderResources(c, 0, 1, &none);

    /* 3. HUD -> target, alpha blended */
    if (show_hud && hud.n) {
        HR(ID3D11DeviceContext_Map(c, (ID3D11Resource*)r->hud_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m));
        memcpy(m.pData, hud.v, (size_t)hud.n * sizeof(hud_vtx));
        ID3D11DeviceContext_Unmap(c, (ID3D11Resource*)r->hud_vb, 0);
        ID3D11DeviceContext_OMSetBlendState(c, r->alpha, blend, 0xffffffff);
        ID3D11DeviceContext_IASetInputLayout(c, r->hud_layout);
        ID3D11DeviceContext_IASetVertexBuffers(c, 0, 1, &r->hud_vb, &hud_stride, &zero);
        ID3D11DeviceContext_VSSetShader(c, r->vs_hud, NULL, 0);
        ID3D11DeviceContext_PSSetShader(c, r->ps_hud, NULL, 0);
        ID3D11DeviceContext_PSSetShaderResources(c, 1, 1, &r->font_srv);
        ID3D11DeviceContext_Draw(c, (UINT)hud.n, 0);
    }
}

/* Read the final image back: BMP file and a not-blank check. */
static int renderer_capture(renderer* r, const char* path) {
    D3D11_TEXTURE2D_DESC td;
    ID3D11Texture2D* staging;
    D3D11_MAPPED_SUBRESOURCE m;
    double sum = 0;
    size_t lit = 0, px = (size_t)r->width * r->height;
    int x, y, bad;
    ID3D11Texture2D_GetDesc(r->target, &td);
    td.Usage = D3D11_USAGE_STAGING; td.BindFlags = 0; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ; td.MiscFlags = 0;
    HR(ID3D11Device_CreateTexture2D(r->dev, &td, NULL, &staging));
    ID3D11DeviceContext_CopyResource(r->ctx, (ID3D11Resource*)staging, (ID3D11Resource*)r->target);
    HR(ID3D11DeviceContext_Map(r->ctx, (ID3D11Resource*)staging, 0, D3D11_MAP_READ, 0, &m));
    for (y = 0; y < r->height; y++) {
        const unsigned char* p = (const unsigned char*)m.pData + (size_t)y * m.RowPitch;
        for (x = 0; x < r->width; x++) {
            int l = (p[4 * x] * 2 + p[4 * x + 1] * 5 + p[4 * x + 2]) / 8;
            sum += l;
            lit += l > 48;
        }
    }
    if (path) {
        if (write_bmp(path, m.pData, (int)m.RowPitch, r->width, r->height) == 0) printf("captured %s\n", path);
        else fprintf(stderr, "could not write %s\n", path);
    }
    ID3D11DeviceContext_Unmap(r->ctx, (ID3D11Resource*)staging, 0);
    RELEASE(staging);
    bad = sum / (double)px < 2.0 || (double)lit / (double)px < 0.005;
    printf("image: mean luminance %.1f / 255, %.1f%% of pixels lit  %s\n", sum / (double)px,
           100.0 * (double)lit / (double)px, bad ? "FAIL (blank?)" : "ok");
    return bad;
}

static void renderer_destroy(renderer* r) {
    RELEASE(r->idle); RELEASE(r->point); RELEASE(r->no_depth); RELEASE(r->rs); RELEASE(r->alpha); RELEASE(r->additive);
    RELEASE(r->cb); RELEASE(r->hud_vb); RELEASE(r->color_vb); RELEASE(r->clip_vb);
    RELEASE(r->hud_layout); RELEASE(r->star_layout);
    RELEASE(r->ps_hud); RELEASE(r->vs_hud); RELEASE(r->ps_tone); RELEASE(r->vs_full);
    RELEASE(r->ps_star); RELEASE(r->vs_star); RELEASE(r->font_srv);
    RELEASE(r->hdr_srv); RELEASE(r->hdr_rtv); RELEASE(r->hdr);
    RELEASE(r->target_rtv); RELEASE(r->target); RELEASE(r->swap);
    RELEASE(r->ctx); RELEASE(r->dev);
}

/* --------------------------------------------------------------- HUD */
typedef struct {
    const options* o;
    const renderer* r;
    const perf_t* perf;
    const swarm_stats* st;
    int mode, threads, cpus, paused;
} hud_state;

static void build_hud(const hud_state* h) {
    static const float white[4] = { 0.92f, 0.95f, 0.92f, 1 }, dim[4] = { 0.55f, 0.6f, 0.58f, 1 },
                       green[4] = { 0.45f, 1.0f, 0.55f, 1 }, amber[4] = { 1.0f, 0.68f, 0.3f, 1 },
                       panel[4] = { 0.02f, 0.035f, 0.03f, 0.72f }, title[4] = { 0.55f, 1.0f, 0.6f, 1 };
    const fp_cpu_info_t* ci = fp_cpu_info();
    const float* mcol[2] = { green, amber };
    const char* brand = ci->brand;
    char nbuf[32];
    float x = 24, y = 22, lh = (float)hud.ch + 3, barx, barw = 170, scale;
    double maxms = 1e-3;
    int s, m;

    while (*brand == ' ') brand++;
    hud.n = 0;
    hud_rect(12, 12, 74.0f * hud.cw, lh * 15 + 18, panel);

    hud_text(x, y, title, "VERDANT SWARM");
    hud_text(x + 15.0f * hud.cw, y, dim, "%s stars  |  %.1f fps (%.2f ms)  |  %s%s",
             thousands(h->o->stars, nbuf), h->perf->frame_ms > 0 ? 1000.0 / h->perf->frame_ms : 0.0,
             h->perf->frame_ms, h->r->driver, h->paused ? "  |  PAUSED" : "");
    y += lh;
    hud_text(x, y, dim, "CPU %.40s  |  %d/%d threads  |  kernels %s%s", brand, h->threads, h->cpus,
             fp_tier_name(fp_dispatch_best_tier()), fp_dispatch_enabled() ? "" : " (static)");
    y += lh * 1.5f;

    hud_text(x, y, white, "simulation running:");
    hud_text(x + 20.0f * hud.cw, y, mcol[h->mode], "%s", swarm_mode_names[h->mode]);
    hud_text(x + 30.0f * hud.cw, y, dim, "(A/B timed every 3rd frame, same data)");
    y += lh * 1.5f;

    hud_text(x, y, dim, "ms / frame     FP-ASM   plain C  speedup");
    y += lh;
    for (m = 0; m < 2; m++)
        for (s = 0; s < SWARM_STAGES; s++) if (h->perf->ms[m][s] > maxms) maxms = h->perf->ms[m][s];
    scale = barw / (float)maxms;
    barx = x + 42.0f * hud.cw;
    for (s = 0; s <= SWARM_STAGES; s++) {
        double a = s < SWARM_STAGES ? h->perf->ms[0][s] : perf_total(h->perf, 0);
        double b = s < SWARM_STAGES ? h->perf->ms[1][s] : perf_total(h->perf, 1);
        const float* name_col = s < SWARM_STAGES ? white : title;
        hud_text(x, y, name_col, "%-12s", s < SWARM_STAGES ? swarm_stage_names[s] : "simulation");
        hud_text(x + 13.0f * hud.cw, y, h->mode == 0 ? green : dim, "%7.2f", a);
        hud_text(x + 22.0f * hud.cw, y, h->mode == 1 ? amber : dim, "%7.2f", b);
        if (a > 0 && b > 0) hud_text(x + 32.0f * hud.cw, y, b >= a ? green : amber, "%6.2fx", b / a);
        if (s < SWARM_STAGES) {
            hud_rect(barx, y + 2, (float)a * scale, (float)hud.ch * 0.38f, green);
            hud_rect(barx, y + 2 + hud.ch * 0.45f, (float)b * scale, (float)hud.ch * 0.38f, amber);
        }
        y += lh;
    }
    y += lh * 0.5f;
    hud_text(x, y, dim, "galaxy  centre (%+.2f %+.2f %+.2f)  spread %.3f  energy %.2e",
             h->st->com.x, h->st->com.y, h->st->com.z, h->st->spread, h->st->energy);
    y += lh;
    hud_text(x, y, dim, "the camera frames the galaxy from these reductions, every frame");
    y += lh * 1.5f;
    hud_text(x, y, white, "[C] FP-ASM/C  [T] threads  [K] kernels  [Space] pause  [H] HUD  [Esc]");
}

/* --------------------------------------------------------------- main */
#define STAR_SCALE 0.45f                  /* sprite radius per unit of star size */
static void camera(Mat4* vp, float* proj_scale, const fp_gfx_conventions* cv, float aspect,
                   const Vec3f* target, float dist, double t) {
    Mat4 v, p;
    float yaw = 0.7f + 0.05f * (float)t, pitch = 0.52f + 0.1f * sinf(0.11f * (float)t);
    float ex = target->x + dist * cosf(pitch) * sinf(yaw);
    float ey = target->y + dist * sinf(pitch);
    float ez = target->z + dist * cosf(pitch) * cosf(yaw);
    fp_mat4_lookat_gfx(&v, ex, ey, ez, target->x, target->y, target->z, 0, 1, 0, cv);
    fp_mat4_perspective_gfx(&p, 0.9f, aspect, 0.05f, INFINITY, cv);  /* reversed-Z, infinite far */
    fp_mat4_mul(vp, &p, &v);
    proj_scale[0] = STAR_SCALE * fabsf(p.m[0]);
    proj_scale[1] = STAR_SCALE * fabsf(p.m[5]);
}

int main(int argc, char** argv) {
    options o;
    renderer r;
    swarm_galaxy g;
    swarm_pool* pool;
    swarm_stats st;
    perf_t perf;
    fp_gfx_conventions cv;
    HWND wnd = NULL;
    Vec3f target = { 0, 0, 0, 0 };
    double t = 0, t_prev = 0, last = 0, start;
    float dist = 18.0f;
    int threads, cpus = swarm_cpu_count(), mode, frame = 0, fail = 0, tier_cap = FP_TIER_AUTO;

    if (parse(&o, argc, argv)) return 2;
    fp_dispatch_init();
    fp_gfx_conventions_init(&cv, FP_GFX_D3D11, 1);
    memset(&perf, 0, sizeof perf);
    memset(&st, 0, sizeof st);
    mode = o.mode;
    threads = o.threads > 0 ? o.threads : cpus;

    printf("Verdant Swarm: %zu stars, %d threads, %s kernels, %s first\n", o.stars, threads,
           fp_tier_name(fp_dispatch_best_tier()), swarm_mode_names[mode]);
    if (swarm_galaxy_create(&g, o.stars, 7)) { fprintf(stderr, "out of memory\n"); return 2; }
    pool = swarm_pool_create(threads);
    if (!o.headless) wnd = make_window(o.width, o.height);
    renderer_init(&r, &o, wnd, &g);
    printf("renderer: Direct3D 11, %s, %dx%d, conventions %s reversed-Z%s\n", r.driver, r.width, r.height,
           fp_gfx_preset_name(cv.preset), r.idle ? " (waits for the rasterizer before simulating)" : "");

    swarm_frame(&g, pool, (swarm_mode)mode, 0, NULL, NULL, &st, NULL);  /* initial stats for the camera */
    keys.hud = 0;
    start = last = swarm_now_ms();

    while (!keys.quit) {
        FrameCB fcb;
        Mat4 vp;
        swarm_timing tm;
        D3D11_MAPPED_SUBRESOURCE m;
        double now;

        if (wnd) {
            MSG msg;
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            if (keys.quit) break;
            if (keys.toggle_mode) { keys.toggle_mode = 0; mode ^= 1; }
            if (keys.next_threads) {               /* 1, 2, 4, ... all */
                keys.next_threads = 0;
                threads = threads >= cpus ? 1 : threads * 2 > cpus ? cpus : threads * 2;
                swarm_pool_destroy(pool);
                pool = swarm_pool_create(threads);
                memset(&perf.seen, 0, sizeof perf.seen);
            }
            if (keys.next_tier) {                  /* auto -> lower tiers that exist -> auto */
                keys.next_tier = 0;
                do tier_cap = tier_cap == FP_TIER_AUTO ? FP_TIER_COUNT - 1 : tier_cap - 1;
                while (tier_cap >= 0 && !fp_dispatch_tier_available((fp_tier)tier_cap));
                if (tier_cap < 0) tier_cap = FP_TIER_AUTO;
                fp_dispatch_set_max_tier((fp_tier)tier_cap);
                memset(&perf.seen, 0, sizeof perf.seen);
            }
        }

        now = swarm_now_ms();
        perf.frame_ms = perf.frame_ms > 0 ? perf.frame_ms * 0.95 + (now - last) * 0.05 : now - last;
        t_prev = t;
        if (o.headless) t = frame / 60.0;          /* fixed timestep: reproducible captures */
        else if (!keys.pause) t += (now - last) * 1e-3;
        last = now;

        /* camera framing from last frame's reductions (centre of mass, spread) */
        target.x += (st.com.x - target.x) * 0.05f;
        target.y += (st.com.y - target.y) * 0.05f;
        target.z += (st.com.z - target.z) * 0.05f;
        dist += (fmaxf(6.0f, st.spread * 5.2f) - dist) * 0.05f;
        camera(&vp, fcb.proj_scale, &cv, (float)o.width / (float)o.height, &target, dist, t);

        renderer_wait_idle(&r);
        /* the frame: projection writes straight into the mapped vertex buffer */
        HR(ID3D11DeviceContext_Map(r.ctx, (ID3D11Resource*)r.clip_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m));
        if (frame % 3 == 2) {
            /* A/B probe: re-run the PREVIOUS frame in one mode, then this frame
             * in the other, on the same data and into the same mapped memory.
             * The order alternates, so neither mode always gets the warm caches
             * or the first touch of the freshly discarded buffer. Both modes
             * compute the same frame, so the animation is undisturbed. */
            int first = (frame / 3) & 1 ? mode : mode ^ 1;
            swarm_frame(&g, pool, (swarm_mode)first, t_prev, &vp, (Vec3f*)m.pData, NULL, &tm);
            if (frame >= WARMUP) perf_add(&perf, first, &tm);
            swarm_frame(&g, pool, (swarm_mode)(first ^ 1), t, &vp, (Vec3f*)m.pData, &st, &tm);
            if (frame >= WARMUP) perf_add(&perf, first ^ 1, &tm);
        } else {
            swarm_frame(&g, pool, (swarm_mode)mode, t, &vp, (Vec3f*)m.pData, &st, NULL);
        }
        ID3D11DeviceContext_Unmap(r.ctx, (ID3D11Resource*)r.clip_vb, 0);

        fcb.min_ndc[0] = 3.0f / (float)o.width;   /* 1.5 px half-size floor */
        fcb.min_ndc[1] = 3.0f / (float)o.height;
        fcb.screen[0] = (float)o.width;
        fcb.screen[1] = (float)o.height;
        fcb.exposure = 0.55f * sqrtf(1048576.0f / (float)o.stars);
        fcb.pad = 0;
        if (!keys.hud) {
            hud_state h = { &o, &r, &perf, &st, mode, threads, cpus, keys.pause };
            build_hud(&h);
        }
        renderer_draw(&r, g.n, &fcb, !keys.hud);
        frame++;

        if (o.frames > 0 && frame >= o.frames) {
            fail = renderer_capture(&r, o.capture);
            keys.quit = 1;
        }
        if (r.swap) IDXGISwapChain_Present(r.swap, o.vsync ? 1 : 0, 0);
    }

    {
        int s;
        double secs = (swarm_now_ms() - start) * 1e-3;
        printf("\n%d frames in %.2f s (%.1f fps incl. rendering)\n", frame, secs, secs > 0 ? frame / secs : 0.0);
        printf("ms / frame (smoothed)  %10s %10s %9s\n", swarm_mode_names[0], swarm_mode_names[1], "speedup");
        for (s = 0; s <= SWARM_STAGES; s++) {
            double a = s < SWARM_STAGES ? perf.ms[0][s] : perf_total(&perf, 0);
            double b = s < SWARM_STAGES ? perf.ms[1][s] : perf_total(&perf, 1);
            printf("  %-20s %10.3f %10.3f %8.2fx\n", s < SWARM_STAGES ? swarm_stage_names[s] : "simulation total",
                   a, b, a > 0 ? b / a : 0.0);
        }
        printf("galaxy: centre (%.3f %.3f %.3f) spread %.3f energy %.3e\n",
               st.com.x, st.com.y, st.com.z, st.spread, st.energy);
    }

    renderer_destroy(&r);
    swarm_pool_destroy(pool);
    swarm_galaxy_destroy(&g);
    if (wnd) DestroyWindow(wnd);
    return fail;
}
