/*
 * swarm-web.js — "Verdant Swarm" in the browser, for the FP-ASM landing page.
 *
 * The same galaxy as examples/swarm (same recipe, same four stages, same data
 * sizes), simulated in plain JavaScript on the page's main thread and drawn
 * with WebGPU, or WebGL 2 / WebGL 1 on systems without it. The per-stage
 * times are measured live and shown next to the native numbers published in
 * the repository (swarm_bench, 1 thread): plain C and FP-ASM.
 *
 * Single-threaded on purpose: worker threads sharing memory need
 * cross-origin isolation headers, which GitHub Pages cannot send. So the
 * comparison is against the native 1-thread numbers.
 *
 * URL options:  ?gfx=webgpu|webgl2|webgl1   ?stars=262144
 */
(function () {
  'use strict';

  // ---- the repository's published numbers (README, "Demo: Verdant Swarm") ----
  var NATIVE = {
    stars: 1048576,
    machine: 'Intel Xeon (Emerald Rapids, AVX-512) · 1 thread · gcc 13 -O3 -march=native',
    c:     [3.11, 1.81, 7.08, 1.92],
    fpasm: [1.44, 1.38, 2.53, 1.56]
  };
  var STAGES = ['motion', 'turbulence', 'analysis', 'projection'];
  var STAGE_SUB = ['quaternion rotation per band', 'world += a(t) · jitter',
                   'centre · spread · energy (reductions)', 'clip = view_proj · world'];
  var GALAXY_R = 10, BANDS = 96, STAR_SCALE = 0.45, WINDOW = 61;

  var $ = function (id) { return document.getElementById(id); };
  var now = function () { return performance.now(); };

  // ------------------------------------------------------------ galaxy
  function rng(seed) {                        // mulberry32
    var s = seed >>> 0;
    return function () {
      s = (s + 0x6D2B79F5) >>> 0;
      var t = s;
      t = Math.imul(t ^ (t >>> 15), t | 1);
      t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }

  function makeGalaxy(n, seed) {
    var u = rng(seed);
    var gauss = function () {
      var a = u() + 1e-7, b = u();
      return Math.sqrt(-2 * Math.log(a)) * Math.cos(2 * Math.PI * b);
    };
    var pos = new Float32Array(4 * n), jit = new Float32Array(4 * n), col = new Float32Array(4 * n);
    var band = new Uint8Array(n), i, k, b;
    for (i = 0; i < n; i++) {
      var r, th, y, bright, size, cr, cg, cb, pick = u();
      if (pick < 0.16) {                                    // bulge: warm, dense, bright
        r = Math.abs(gauss()) * 1.1;
        th = u() * 2 * Math.PI;
        y = gauss() * 0.55 * Math.exp(-r * 0.4);
        cr = 1; cg = 0.82 + 0.1 * u(); cb = 0.55 + 0.15 * u();
        bright = 0.35 + 0.65 * u();
        size = 0.035 + 0.04 * u();
      } else {                                              // disk with 4 logarithmic arms
        var arm = Math.floor(u() * 4) & 3;
        r = 0.6 - Math.log(u() + 1e-6) * 2.6;
        if (r > GALAXY_R) r = GALAXY_R * u() + 0.6;
        var ja = gauss() * (0.18 + 0.05 * r);
        th = arm * (Math.PI * 0.5) + Math.log(r) * 2.3 + ja;
        y = gauss() * 0.12 * Math.exp(-r * 0.12);
        pick = u();
        if (pick < 0.35)      { cr = 0.55; cg = 0.7;  cb = 1.0; }   // young, blue
        else if (pick < 0.75) { cr = 1.0;  cg = 0.95; cb = 0.88; }  // white
        else if (pick < 0.92) { cr = 1.0;  cg = 0.75; cb = 0.5; }   // old, orange
        else                  { cr = 0.9;  cg = 0.35; cb = 0.45; }  // nebula pink
        bright = 0.15 + 0.85 * u() * Math.exp(-Math.abs(ja) * 2);
        size = 0.02 + 0.05 * u();
        if (u() < 0.004) { size *= 4; bright = 1; }                  // rare giants
      }
      k = 4 * i;
      pos[k] = r * Math.cos(th); pos[k + 1] = y; pos[k + 2] = r * Math.sin(th);
      jit[k] = gauss() * 0.05; jit[k + 1] = gauss() * 0.05; jit[k + 2] = gauss() * 0.05;
      col[k] = cr * bright; col[k + 1] = cg * bright; col[k + 2] = cb * bright; col[k + 3] = size;
      b = Math.floor(r / (GALAXY_R + 1) * BANDS);
      band[i] = b < 0 ? 0 : b >= BANDS ? BANDS - 1 : b;
    }
    // sort stars by band (counting sort): each band is one contiguous batch
    var start = new Uint32Array(BANDS + 1);
    for (i = 0; i < n; i++) start[band[i] + 1]++;
    for (b = 0; b < BANDS; b++) start[b + 1] += start[b];
    var fill = start.slice(0, BANDS);
    var base = new Float32Array(4 * n), jitter = new Float32Array(4 * n), color = new Float32Array(4 * n);
    for (i = 0; i < n; i++) {
      var d = 4 * fill[band[i]]++, s = 4 * i;
      base[d] = pos[s]; base[d + 1] = pos[s + 1]; base[d + 2] = pos[s + 2];
      jitter[d] = jit[s]; jitter[d + 1] = jit[s + 1]; jitter[d + 2] = jit[s + 2];
      color[d] = col[s]; color[d + 1] = col[s + 1]; color[d + 2] = col[s + 2]; color[d + 3] = col[s + 3];
    }
    var omega = new Float32Array(BANDS);
    for (b = 0; b < BANDS; b++) {
      var rb = (b + 0.5) / BANDS * (GALAXY_R + 1);
      omega[b] = 0.06 + 0.42 / (1 + rb * 0.45);
    }
    return { n: n, start: start, omega: omega, base: base, jitter: jitter, color: color,
             world: base.slice(), prev: base.slice(), frames: 0 };
  }

  // One frame at time t, the same four stages as swarm_frame() in plain C.
  // Writes n clip-space float4 positions into clip, stats and per-stage ms.
  function simulate(g, t, m, clip, st, ms) {
    var tmp = g.prev; g.prev = g.world; g.world = tmp;
    var W = g.world, P = g.prev, B = g.base, J = g.jitter, e = 4 * g.n, k, b;
    var t0 = now();

    // 1. motion: per band, rotate base by a quaternion about +y (same formula as c_rotate)
    for (b = 0; b < BANDS; b++) {
      var h = g.omega[b] * t * 0.5, qx = 0, qy = Math.sin(h), qz = 0, qw = Math.cos(h), end = 4 * g.start[b + 1];
      for (k = 4 * g.start[b]; k < end; k += 4) {
        var x = B[k], y = B[k + 1], z = B[k + 2];
        var tx = 2 * (qy * z - qz * y), ty = 2 * (qz * x - qx * z), tz = 2 * (qx * y - qy * x);
        W[k]     = x + qw * tx + (qy * tz - qz * ty);
        W[k + 1] = y + qw * ty + (qz * tx - qx * tz);
        W[k + 2] = z + qw * tz + (qx * ty - qy * tx);
        W[k + 3] = 0;
      }
    }
    var t1 = now();

    // 2. turbulence: world = a * jitter + world over all 4n floats (axpy)
    var a = 0.6 * Math.sin(t * 0.9) + 0.3 * Math.sin(t * 2.3);
    for (k = 0; k < e; k++) W[k] = a * J[k] + W[k];
    var t2 = now();

    // 3. analysis: three reductions (vector sum, sum of squares, squared difference)
    var sx = 0, sy = 0, sz = 0, sq = 0, en = 0, v;
    for (k = 0; k < e; k += 4) { sx += W[k]; sy += W[k + 1]; sz += W[k + 2]; }
    for (k = 0; k < e; k++) { v = W[k]; sq += v * v; }
    for (k = 0; k < e; k++) { v = W[k] - P[k]; en += v * v; }
    var t3 = now();

    // 4. projection: clip = view_proj * (world, 1)
    var m0 = m[0], m1 = m[1], m2 = m[2], m3 = m[3], m4 = m[4], m5 = m[5], m6 = m[6], m7 = m[7],
        m8 = m[8], m9 = m[9], m10 = m[10], m11 = m[11], m12 = m[12], m13 = m[13], m14 = m[14], m15 = m[15];
    for (k = 0; k < e; k += 4) {
      var px = W[k], py = W[k + 1], pz = W[k + 2];
      clip[k]     = m0 * px + m4 * py + m8 * pz + m12;
      clip[k + 1] = m1 * px + m5 * py + m9 * pz + m13;
      clip[k + 2] = m2 * px + m6 * py + m10 * pz + m14;
      clip[k + 3] = m3 * px + m7 * py + m11 * pz + m15;
    }
    var t4 = now();

    ms[0] = t1 - t0; ms[1] = t2 - t1; ms[2] = t3 - t2; ms[3] = t4 - t3;
    var inv = 1 / g.n, cx = sx * inv, cy = sy * inv, cz = sz * inv;
    var r2 = sq * inv - (cx * cx + cy * cy + cz * cz);
    st.com[0] = cx; st.com[1] = cy; st.com[2] = cz;
    st.spread = Math.sqrt(r2 > 0 ? r2 : 0);
    st.energy = g.frames ? en * inv : 0;
    g.frames++;
  }

  // ------------------------------------------------- camera (fp_gfx D3D11)
  // Right-handed look-at and a reversed-Z, infinite-far perspective with
  // [0,1] depth: the FP_GFX_D3D11 convention. WebGPU uses the same clip
  // space; WebGL clips -w..w, which this matrix also satisfies (z = near).
  function lookAt(e, c) {
    var fx = c[0] - e[0], fy = c[1] - e[1], fz = c[2] - e[2];
    var fl = Math.hypot(fx, fy, fz); fx /= fl; fy /= fl; fz /= fl;
    var sx = -fz, sy = 0, sz = fx;                       // f x (0,1,0)
    var sl = Math.hypot(sx, sy, sz); sx /= sl; sz /= sl;
    var ux = sy * fz - sz * fy, uy = sz * fx - sx * fz, uz = sx * fy - sy * fx;
    return [sx, ux, -fx, 0,  sy, uy, -fy, 0,  sz, uz, -fz, 0,
            -(sx * e[0] + sy * e[1] + sz * e[2]), -(ux * e[0] + uy * e[1] + uz * e[2]), fx * e[0] + fy * e[1] + fz * e[2], 1];
  }
  function perspective(fovy, aspect, near) {
    var f = 1 / Math.tan(fovy / 2);
    return [f / aspect, 0, 0, 0,  0, f, 0, 0,  0, 0, 0, -1,  0, 0, near, 0];
  }
  function mul(a, b) {
    var o = new Array(16), r, c;
    for (c = 0; c < 4; c++)
      for (r = 0; r < 4; r++)
        o[c * 4 + r] = a[r] * b[c * 4] + a[4 + r] * b[c * 4 + 1] + a[8 + r] * b[c * 4 + 2] + a[12 + r] * b[c * 4 + 3];
    return o;
  }

  // ---------------------------------------------------------- shaders
  var WGSL = [
    'struct U { proj_scale: vec2f, min_ndc: vec2f, screen: vec2f, exposure: f32, pad: f32 };',
    '@group(0) @binding(0) var<uniform> u: U;',
    '@group(0) @binding(1) var hdr: texture_2d<f32>;',
    'struct V { @builtin(position) pos: vec4f, @location(0) uv: vec2f, @location(1) col: vec3f };',
    '@vertex fn vs_star(@builtin(vertex_index) vi: u32, @location(0) clip: vec4f, @location(1) col: vec4f) -> V {',
    '  let c = vec2f(select(-1.0, 1.0, (vi & 1u) != 0u), select(-1.0, 1.0, (vi & 2u) != 0u));',
    '  let w = col.w * u.proj_scale;',
    '  let f = u.min_ndc * clip.w;',
    '  let k = clamp(w.x / max(f.x, 1e-6), 0.0, 1.0);',
    '  var o: V;',
    '  o.pos = vec4f(clip.xy + c * max(w, f), clip.z, clip.w);',
    '  o.uv = c;',
    '  o.col = col.rgb * (k * k);',
    '  return o;',
    '}',
    '@fragment fn fs_star(i: V) -> @location(0) vec4f {',
    '  let a = max(exp(-4.0 * dot(i.uv, i.uv)) - 0.0183, 0.0);',
    '  return vec4f(i.col * a * u.exposure, 1.0);',
    '}',
    '@vertex fn vs_full(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {',
    '  let p = vec2f(f32((vi << 1u) & 2u), f32(vi & 2u));',
    '  return vec4f(p * vec2f(2.0, -2.0) + vec2f(-1.0, 1.0), 0.0, 1.0);',
    '}',
    '@fragment fn fs_tone(@builtin(position) p: vec4f) -> @location(0) vec4f {',
    '  var c = textureLoad(hdr, vec2i(p.xy), 0).rgb;',
    '  let q = p.xy / u.screen - 0.5;',
    '  c += vec3f(0.004, 0.006, 0.014) * (1.0 - dot(q, q));',
    '  c = 1.0 - exp(-c);',
    '  return vec4f(pow(c, vec3f(1.0 / 2.2)), 1.0);',
    '}'
  ].join('\n');

  // GLSL ES 1.00: valid in WebGL 1 and WebGL 2
  var VS_STAR = [
    'attribute vec2 corner; attribute vec4 clip; attribute vec4 col;',
    'uniform vec2 proj_scale; uniform vec2 min_ndc;',
    'varying vec2 v_uv; varying vec3 v_col;',
    'void main() {',
    '  vec2 w = col.w * proj_scale; vec2 f = min_ndc * clip.w;',
    '  float k = clamp(w.x / max(f.x, 1e-6), 0.0, 1.0);',
    '  gl_Position = clip + vec4(corner * max(w, f), 0.0, 0.0);',
    '  v_uv = corner; v_col = col.rgb * (k * k);',
    '}'].join('\n');
  var FS_STAR = [
    'precision mediump float;',
    'varying vec2 v_uv; varying vec3 v_col; uniform float exposure;',
    'void main() {',
    '  float a = max(exp(-4.0 * dot(v_uv, v_uv)) - 0.0183, 0.0);',
    '  gl_FragColor = vec4(v_col * a * exposure, 1.0);',
    '}'].join('\n');
  var VS_FULL = 'attribute vec2 p; void main() { gl_Position = vec4(p, 0.0, 1.0); }';
  var FS_TONE = [
    'precision mediump float;',
    'uniform sampler2D hdr; uniform vec2 screen;',
    'void main() {',
    '  vec2 uv = gl_FragCoord.xy / screen;',
    '  vec3 c = texture2D(hdr, uv).rgb;',
    '  vec2 q = uv - 0.5;',
    '  c += vec3(0.004, 0.006, 0.014) * (1.0 - dot(q, q));',
    '  c = 1.0 - exp(-c);',
    '  gl_FragColor = vec4(pow(c, vec3(1.0 / 2.2)), 1.0);',
    '}'].join('\n');

  // --------------------------------------------------------- WebGPU
  async function createWebGPU(canvas) {
    if (!navigator.gpu) throw new Error('not supported by this browser');
    var adapter = await navigator.gpu.requestAdapter({ powerPreference: 'high-performance' });
    if (!adapter) throw new Error('no adapter');
    var device = await adapter.requestDevice();
    var ctx = canvas.getContext('webgpu');
    if (!ctx) throw new Error('no canvas context');
    var format = navigator.gpu.getPreferredCanvasFormat();
    ctx.configure({ device: device, format: format, alphaMode: 'opaque' });
    device.pushErrorScope('validation');
    var mod = device.createShaderModule({ code: WGSL });
    var ubuf = device.createBuffer({ size: 32, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST });
    var add = { srcFactor: 'one', dstFactor: 'one', operation: 'add' };
    var inst = function (loc) {
      return { arrayStride: 16, stepMode: 'instance', attributes: [{ shaderLocation: loc, offset: 0, format: 'float32x4' }] };
    };
    var star = device.createRenderPipeline({
      layout: 'auto',
      vertex: { module: mod, entryPoint: 'vs_star', buffers: [inst(0), inst(1)] },
      fragment: { module: mod, entryPoint: 'fs_star', targets: [{ format: 'rgba16float', blend: { color: add, alpha: add } }] },
      primitive: { topology: 'triangle-strip' }
    });
    var tone = device.createRenderPipeline({
      layout: 'auto',
      vertex: { module: mod, entryPoint: 'vs_full' },
      fragment: { module: mod, entryPoint: 'fs_tone', targets: [{ format: format }] },
      primitive: { topology: 'triangle-list' }
    });
    var starBG = device.createBindGroup({ layout: star.getBindGroupLayout(0), entries: [{ binding: 0, resource: { buffer: ubuf } }] });
    var err = await device.popErrorScope();
    if (err) throw new Error(err.message);
    var hdr = null, toneBG = null, clipBuf = null, colorBuf = null, lost = null;
    device.lost.then(function (info) { lost = info.message || 'device lost'; });

    return {
      name: 'WebGPU', hdr: true,
      resize: function (w, h) {
        if (hdr) hdr.destroy();
        hdr = device.createTexture({ size: [w, h], format: 'rgba16float',
                                     usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING });
        toneBG = device.createBindGroup({ layout: tone.getBindGroupLayout(0), entries: [
          { binding: 0, resource: { buffer: ubuf } }, { binding: 1, resource: hdr.createView() }] });
      },
      setStars: function (n, color) {
        if (clipBuf) { clipBuf.destroy(); colorBuf.destroy(); }
        clipBuf = device.createBuffer({ size: 16 * n, usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST });
        colorBuf = device.createBuffer({ size: 16 * n, usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST });
        device.queue.writeBuffer(colorBuf, 0, color);
      },
      draw: function (clip, n, U) {
        if (lost) throw new Error('WebGPU ' + lost);
        device.queue.writeBuffer(ubuf, 0, U);
        device.queue.writeBuffer(clipBuf, 0, clip, 0, 4 * n);
        var enc = device.createCommandEncoder();
        var p = enc.beginRenderPass({ colorAttachments: [{ view: hdr.createView(), clearValue: { r: 0, g: 0, b: 0, a: 0 },
                                                           loadOp: 'clear', storeOp: 'store' }] });
        p.setPipeline(star); p.setBindGroup(0, starBG);
        p.setVertexBuffer(0, clipBuf); p.setVertexBuffer(1, colorBuf);
        p.draw(4, n); p.end();
        p = enc.beginRenderPass({ colorAttachments: [{ view: ctx.getCurrentTexture().createView(),
                                                       clearValue: { r: 0, g: 0, b: 0, a: 1 }, loadOp: 'clear', storeOp: 'store' }] });
        p.setPipeline(tone); p.setBindGroup(0, toneBG); p.draw(3); p.end();
        device.queue.submit([enc.finish()]);
      },
      destroy: function () { if (hdr) hdr.destroy(); if (clipBuf) { clipBuf.destroy(); colorBuf.destroy(); } device.destroy(); }
    };
  }

  // ---------------------------------------------------------- WebGL
  function createWebGL(canvas, version) {
    var opts = { alpha: false, antialias: false, depth: false, stencil: false, premultipliedAlpha: false,
                 powerPreference: 'high-performance' };
    var gl = version === 2 ? canvas.getContext('webgl2', opts)
                           : (canvas.getContext('webgl', opts) || canvas.getContext('experimental-webgl', opts));
    if (!gl) throw new Error('not supported');
    var divisor, drawInstanced, hdrType = null, hdrFormat = null;
    if (version === 2) {
      divisor = function (l, d) { gl.vertexAttribDivisor(l, d); };
      drawInstanced = function (c, n) { gl.drawArraysInstanced(gl.TRIANGLE_STRIP, 0, c, n); };
      if (gl.getExtension('EXT_color_buffer_float') || gl.getExtension('EXT_color_buffer_half_float')) {
        hdrType = gl.HALF_FLOAT; hdrFormat = gl.RGBA16F;
      }
    } else {
      var ia = gl.getExtension('ANGLE_instanced_arrays');
      if (!ia) throw new Error('no instancing (ANGLE_instanced_arrays)');
      divisor = function (l, d) { ia.vertexAttribDivisorANGLE(l, d); };
      drawInstanced = function (c, n) { ia.drawArraysInstancedANGLE(gl.TRIANGLE_STRIP, 0, c, n); };
      var hf = gl.getExtension('OES_texture_half_float');
      if (hf && gl.getExtension('EXT_color_buffer_half_float')) { hdrType = hf.HALF_FLOAT_OES; hdrFormat = gl.RGBA; }
    }

    function program(vs, fs, attribs) {
      var p = gl.createProgram();
      [[gl.VERTEX_SHADER, vs], [gl.FRAGMENT_SHADER, fs]].forEach(function (s) {
        var sh = gl.createShader(s[0]);
        gl.shaderSource(sh, s[1]); gl.compileShader(sh);
        if (!gl.getShaderParameter(sh, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(sh));
        gl.attachShader(p, sh);
      });
      attribs.forEach(function (a, i) { gl.bindAttribLocation(p, i, a); });
      gl.linkProgram(p);
      if (!gl.getProgramParameter(p, gl.LINK_STATUS)) throw new Error(gl.getProgramInfoLog(p));
      return p;
    }
    var starProg = program(VS_STAR, FS_STAR, ['corner', 'clip', 'col']);
    var toneProg = program(VS_FULL, FS_TONE, ['p']);
    var loc = {
      proj_scale: gl.getUniformLocation(starProg, 'proj_scale'), min_ndc: gl.getUniformLocation(starProg, 'min_ndc'),
      exposure: gl.getUniformLocation(starProg, 'exposure'),
      hdr: gl.getUniformLocation(toneProg, 'hdr'), screen: gl.getUniformLocation(toneProg, 'screen')
    };
    var cornerBuf = gl.createBuffer(), fullBuf = gl.createBuffer(), clipBuf = gl.createBuffer(), colorBuf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, cornerBuf);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);
    gl.bindBuffer(gl.ARRAY_BUFFER, fullBuf);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 3, -1, -1, 3]), gl.STATIC_DRAW);
    var tex = null, fbo = null, hdrOk = false, W = 1, H = 1;

    function attrib(i, buf, size, div) {
      gl.bindBuffer(gl.ARRAY_BUFFER, buf);
      gl.enableVertexAttribArray(i);
      gl.vertexAttribPointer(i, size, gl.FLOAT, false, 0, 0);
      divisor(i, div);
    }

    return {
      name: version === 2 ? 'WebGL 2' : 'WebGL 1', hdr: false,
      resize: function (w, h) {
        W = w; H = h; hdrOk = false;
        if (tex) { gl.deleteTexture(tex); gl.deleteFramebuffer(fbo); tex = fbo = null; }
        if (hdrType !== null) {
          tex = gl.createTexture();
          gl.bindTexture(gl.TEXTURE_2D, tex);
          gl.texImage2D(gl.TEXTURE_2D, 0, hdrFormat, w, h, 0, gl.RGBA, hdrType, null);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
          gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
          fbo = gl.createFramebuffer();
          gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
          gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);
          hdrOk = gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE;
          gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        }
        this.hdr = hdrOk;
      },
      setStars: function (n, color) {
        gl.bindBuffer(gl.ARRAY_BUFFER, clipBuf);
        gl.bufferData(gl.ARRAY_BUFFER, 16 * n, gl.DYNAMIC_DRAW);
        gl.bindBuffer(gl.ARRAY_BUFFER, colorBuf);
        gl.bufferData(gl.ARRAY_BUFFER, color, gl.STATIC_DRAW);
      },
      draw: function (clip, n, U) {
        if (gl.isContextLost()) throw new Error(this.name + ' context lost');
        gl.bindBuffer(gl.ARRAY_BUFFER, clipBuf);
        gl.bufferSubData(gl.ARRAY_BUFFER, 0, clip.subarray(0, 4 * n));
        gl.bindFramebuffer(gl.FRAMEBUFFER, hdrOk ? fbo : null);
        gl.viewport(0, 0, W, H);
        if (hdrOk) gl.clearColor(0, 0, 0, 0); else gl.clearColor(0.012, 0.016, 0.03, 1);
        gl.clear(gl.COLOR_BUFFER_BIT);
        gl.enable(gl.BLEND);
        gl.blendFunc(gl.ONE, gl.ONE);
        gl.useProgram(starProg);
        gl.uniform2f(loc.proj_scale, U[0], U[1]);
        gl.uniform2f(loc.min_ndc, U[2], U[3]);
        gl.uniform1f(loc.exposure, hdrOk ? U[6] : U[6] * 0.7);
        attrib(0, cornerBuf, 2, 0);
        attrib(1, clipBuf, 4, 1);
        attrib(2, colorBuf, 4, 1);
        drawInstanced(4, n);
        gl.disableVertexAttribArray(1); gl.disableVertexAttribArray(2);
        gl.disable(gl.BLEND);
        if (hdrOk) {
          gl.bindFramebuffer(gl.FRAMEBUFFER, null);
          gl.useProgram(toneProg);
          gl.activeTexture(gl.TEXTURE0);
          gl.bindTexture(gl.TEXTURE_2D, tex);
          gl.uniform1i(loc.hdr, 0);
          gl.uniform2f(loc.screen, W, H);
          attrib(0, fullBuf, 2, 0);
          gl.drawArrays(gl.TRIANGLES, 0, 3);
        }
      },
      destroy: function () { var x = gl.getExtension('WEBGL_lose_context'); if (x) x.loseContext(); }
    };
  }

  // ------------------------------------------------------- the demo
  var ui = {
    section: $('demo'), stage: $('swarm-stage'), hud: $('swarm-hud'), msg: $('swarm-msg'),
    stars: $('swarm-stars'), gfx: $('swarm-gfx'), pause: $('swarm-pause'), hudBtn: $('swarm-hudbtn'),
    rows: $('swarm-rows'), summary: $('swarm-summary'), machine: $('swarm-machine')
  };
  if (!ui.stage) return;

  var params = new URLSearchParams(location.search);
  var S = {
    g: null, clip: null, r: null, canvas: null, paused: false, hud: true, t: 0, last: 0,
    target: [0, 0, 0], dist: 18, st: { com: [0, 0, 0], spread: 0, energy: 0 },
    visible: false, running: false, busy: false, fps: 0, lastUi: 0,
    hist: [], histPos: 0, histN: 0, ms: [0, 0, 0, 0], med: [0, 0, 0, 0, 0], errors: []
  };
  for (var h = 0; h < 5; h++) S.hist.push(new Float64Array(WINDOW));
  // exposed for automated checks
  window.__swarm = { ready: false, renderer: null, n: 0, frames: 0, fps: 0, ms: S.med, errors: S.errors };

  function fmtInt(n) { return n.toLocaleString('en-US'); }
  // Trimmed mean (drop the fastest and slowest 10%). Without cross-origin
  // isolation, browsers coarsen and jitter performance.now(); a median would
  // snap to the timer's resolution, while a mean averages the jitter out.
  function robustMean(a, n) {
    var b = Array.prototype.slice.call(a, 0, n).sort(function (x, y) { return x - y; });
    var cut = Math.floor(n / 10), sum = 0, i;
    for (i = cut; i < n - cut; i++) sum += b[i];
    return sum / Math.max(1, n - 2 * cut);
  }
  function message(text) { ui.msg.textContent = text || ''; ui.msg.style.display = text ? 'flex' : 'none'; }

  function defaultStars() {
    var p = parseInt(params.get('stars'), 10);
    if (p > 0) return Math.max(1024, Math.min(p, 4194304));
    var mobile = /Mobi|Android|iPhone|iPad/i.test(navigator.userAgent);
    return !mobile && (navigator.hardwareConcurrency || 4) >= 8 ? 1048576 : 262144;
  }

  function freshCanvas() {
    if (S.canvas) S.canvas.remove();
    var c = document.createElement('canvas');
    c.id = 'swarm-canvas';
    c.tabIndex = 0;
    c.setAttribute('aria-label', 'Live galaxy simulation: ' + (S.g ? fmtInt(S.g.n) : '') + ' stars');
    ui.stage.insertBefore(c, ui.stage.firstChild);
    S.canvas = c;
    c.addEventListener('keydown', onKey);
    return c;
  }

  function sizeCanvas() {
    if (!S.canvas || !S.r) return;
    var dpr = Math.min(window.devicePixelRatio || 1, 2);
    var w = Math.max(64, Math.round(ui.stage.clientWidth * dpr)), hh = Math.max(64, Math.round(ui.stage.clientHeight * dpr));
    if (S.canvas.width === w && S.canvas.height === hh && S.sized) return;
    S.canvas.width = w; S.canvas.height = hh; S.sized = true;
    S.r.resize(w, hh);
  }

  async function makeRenderer(kind) {
    var order = kind === 'auto' ? ['webgpu', 'webgl2', 'webgl1'] : [kind];
    var tried = [];
    for (var i = 0; i < order.length; i++) {
      var c = freshCanvas();
      try {
        var r = order[i] === 'webgpu' ? await createWebGPU(c) : createWebGL(c, order[i] === 'webgl2' ? 2 : 1);
        if (tried.length) S.errors.push('fell back to ' + r.name + ' (' + tried.join('; ') + ')');
        return r;
      } catch (e) {
        tried.push((order[i] === 'webgpu' ? 'WebGPU' : order[i] === 'webgl2' ? 'WebGL 2' : 'WebGL 1') + ': ' + e.message);
      }
    }
    throw new Error(tried.join('; '));
  }

  async function setRenderer(kind) {
    if (S.r) { try { S.r.destroy(); } catch (e) { /* already gone */ } S.r = null; }
    S.sized = false;
    try {
      S.r = await makeRenderer(kind);
    } catch (e) {
      S.errors.push(e.message);
      if (S.canvas) S.canvas.remove();
      S.canvas = null;
      ui.stage.classList.add('nogpu');
      message('No WebGPU or WebGL here (' + e.message + '). Showing the native D3D11 capture instead.');
      return false;
    }
    sizeCanvas();
    S.r.setStars(S.g.n, S.g.color);
    window.__swarm.renderer = S.r.name + (S.r.hdr ? '' : ' (LDR)');
    return true;
  }

  function setStars(n) {
    message('Generating ' + fmtInt(n) + ' stars…');
    return new Promise(function (resolve) {
      setTimeout(function () {                 // let the message paint first
        S.g = makeGalaxy(n, 7);
        S.clip = new Float32Array(4 * n);
        S.histN = S.histPos = 0;
        S.t = 0; S.target = [0, 0, 0]; S.dist = 18;
        simulate(S.g, 0, perspective(0.9, 16 / 9, 0.05), S.clip, S.st, S.ms);   // initial stats
        if (S.r) S.r.setStars(n, S.g.color);
        window.__swarm.n = n;
        message('');
        resolve();
      }, 30);
    });
  }

  function onKey(e) {
    if (e.key === ' ') { togglePause(); e.preventDefault(); }
    else if (e.key === 'h' || e.key === 'H') toggleHud();
  }
  function togglePause() { S.paused = !S.paused; ui.pause.textContent = S.paused ? 'Resume' : 'Pause'; }
  function toggleHud() { S.hud = !S.hud; ui.hud.style.display = S.hud ? '' : 'none'; ui.hudBtn.textContent = S.hud ? 'Hide HUD' : 'Show HUD'; }

  var U = new Float32Array(8);
  function tick(ts) {
    requestAnimationFrame(tick);
    var raw = S.last ? ts - S.last : 16, dt = Math.min(raw, 100);
    S.last = ts;
    if (!S.visible || S.busy || !S.r || !S.g) return;
    if (!S.paused) S.t += dt * 1e-3;
    S.fps = S.fps ? S.fps * 0.9 + (1000 / Math.max(raw, 1)) * 0.1 : 1000 / Math.max(raw, 1);
    sizeCanvas();

    // camera framed by last frame's reductions (as in the native demo)
    var st = S.st, k = 0.05;
    S.target[0] += (st.com[0] - S.target[0]) * k;
    S.target[1] += (st.com[1] - S.target[1]) * k;
    S.target[2] += (st.com[2] - S.target[2]) * k;
    S.dist += (Math.max(6, st.spread * 5.2) - S.dist) * k;
    var yaw = 0.7 + 0.05 * S.t, pitch = 0.52 + 0.1 * Math.sin(0.11 * S.t), d = S.dist;
    var eye = [S.target[0] + d * Math.cos(pitch) * Math.sin(yaw), S.target[1] + d * Math.sin(pitch),
               S.target[2] + d * Math.cos(pitch) * Math.cos(yaw)];
    var w = S.canvas.width, hgt = S.canvas.height;
    var P = perspective(0.9, w / hgt, 0.05), VP = mul(P, lookAt(eye, S.target));

    simulate(S.g, S.t, VP, S.clip, S.st, S.ms);

    var tot = 0;
    for (var s = 0; s < 4; s++) { S.hist[s][S.histPos] = S.ms[s]; tot += S.ms[s]; }
    S.hist[4][S.histPos] = tot;
    S.histPos = (S.histPos + 1) % WINDOW;
    if (S.histN < WINDOW) S.histN++;

    U[0] = STAR_SCALE * P[0]; U[1] = STAR_SCALE * P[5];
    U[2] = 3 / w; U[3] = 3 / hgt; U[4] = w; U[5] = hgt;
    U[6] = 0.55 * Math.sqrt(1048576 / S.g.n); U[7] = 0;
    try {
      S.r.draw(S.clip, S.g.n, U);
    } catch (e) {
      S.errors.push(e.message);
      S.busy = true;
      message(e.message + ' — switching renderer…');
      setRenderer('auto').then(function () { S.busy = false; message(''); });
      return;
    }
    window.__swarm.frames++;
    window.__swarm.fps = S.fps;
    window.__swarm.ready = true;
    if (ts - S.lastUi > 250) { S.lastUi = ts; updateUi(); }
  }

  // ---------------------------------------------------- HUD and table
  function updateUi() {
    var n = S.g.n, i, s;
    for (s = 0; s < 5; s++) S.med[s] = robustMean(S.hist[s], S.histN);
    var st = S.st, sign = function (v) { return (v >= 0 ? '+' : '') + v.toFixed(2); };
    ui.hud.innerHTML =
      '<b>VERDANT SWARM</b> · ' + fmtInt(n) + ' stars<br>' +
      S.fps.toFixed(0) + ' fps · ' + S.r.name + (S.r.hdr ? ' · HDR' : ' · LDR') + (S.paused ? ' · paused' : '') + '<br>' +
      'simulation <b>' + S.med[4].toFixed(2) + ' ms</b> · JavaScript, 1 thread<br>' +
      '<span class="dim">centre (' + sign(st.com[0]) + ' ' + sign(st.com[1]) + ' ' + sign(st.com[2]) + ') · spread ' +
      st.spread.toFixed(3) + ' · energy ' + st.energy.toExponential(2) + '</span>';

    var scale = n / NATIVE.stars, rows = ui.rows.children, max = 0, vals = [];
    for (s = 0; s < 5; s++) {
      var js = S.med[s];
      var c = s < 4 ? NATIVE.c[s] * scale : (NATIVE.c[0] + NATIVE.c[1] + NATIVE.c[2] + NATIVE.c[3]) * scale;
      var f = s < 4 ? NATIVE.fpasm[s] * scale : (NATIVE.fpasm[0] + NATIVE.fpasm[1] + NATIVE.fpasm[2] + NATIVE.fpasm[3]) * scale;
      vals.push([js, c, f]);
      if (s < 4) max = Math.max(max, js, c, f);
    }
    for (s = 0; s < 5; s++) {
      var row = rows[s], v = vals[s], m = s < 4 ? max : Math.max(v[0], v[1], v[2]);
      var bars = row.querySelectorAll('.bar-el');
      for (i = 0; i < 3; i++) {
        bars[i].style.width = (v[i] / m * 78).toFixed(1) + '%';
        bars[i].firstChild.textContent = v[i].toFixed(2) + ' ms';
      }
      var pill = row.querySelector('.speedup'), ratio = v[0] / v[2];
      pill.textContent = ratio >= 1 ? ratio.toFixed(1) + '×' : (1 / ratio).toFixed(1) + '× JS';
      pill.title = ratio >= 1 ? 'native FP-ASM is ' + ratio.toFixed(1) + '× faster than this page\'s JavaScript'
                              : 'this page\'s JavaScript is ' + (1 / ratio).toFixed(1) + '× faster than the native FP-ASM number';
      pill.classList.toggle('par', ratio < 1.2);
    }
    var t = vals[4];
    ui.summary.innerHTML = 'Your browser simulates <b>' + fmtInt(n) + '</b> stars in <b>' + t[0].toFixed(2) +
      ' ms</b> per frame. Natively, the same frame takes <b>' + t[1].toFixed(2) + ' ms</b> in plain C and <b>' +
      t[2].toFixed(2) + ' ms</b> with FP·ASM: <b>' + (t[0] / t[2]).toFixed(1) + '×</b> faster than this page, <b>' +
      (t[1] / t[2]).toFixed(1) + '×</b> faster than C.';
  }

  function buildTable() {
    var html = '';
    for (var s = 0; s < 5; s++) {
      var name = s < 4 ? STAGES[s] : 'whole frame', sub = s < 4 ? STAGE_SUB[s] : 'sum of the four stages';
      html += '<div class="row' + (s === 4 ? ' total' : '') + '"><div class="name">' + name + '<small>' + sub + '</small></div>' +
        '<div><div class="rowtop"><div class="bars" style="flex:1">' +
        '<div class="track"><div class="bar-el js" style="width:0"><span class="bar-val">–</span></div></div>' +
        '<div class="track"><div class="bar-el scalar" style="width:0"><span class="bar-val">–</span></div></div>' +
        '<div class="track"><div class="bar-el asm" style="width:0"><span class="bar-val">–</span></div></div>' +
        '</div><div class="speedup">–</div></div></div></div>';
    }
    ui.rows.innerHTML = html;
    ui.machine.textContent = NATIVE.machine;
  }

  async function start() {
    if (S.running) return;
    S.running = true;
    buildTable();
    var n = defaultStars();
    if (![131072, 262144, 524288, 1048576, 2097152].includes(n)) {
      var o = document.createElement('option'); o.value = n; o.textContent = fmtInt(n); ui.stars.appendChild(o);
    }
    ui.stars.value = String(n);
    var g = params.get('gfx');
    ui.gfx.value = ['webgpu', 'webgl2', 'webgl1'].indexOf(g) >= 0 ? g : 'auto';
    await setStars(n);
    if (!(await setRenderer(ui.gfx.value))) return;
    if (ui.gfx.value === 'auto') ui.gfx.querySelector('option[value=auto]').textContent = 'auto (' + S.r.name + ')';
    requestAnimationFrame(tick);
  }

  ui.stars.addEventListener('change', function () {
    S.busy = true;
    setStars(parseInt(ui.stars.value, 10)).then(function () { S.busy = false; });
  });
  ui.gfx.addEventListener('change', function () {
    S.busy = true;
    setRenderer(ui.gfx.value).then(function (ok) {
      if (ok && ui.gfx.value === 'auto') ui.gfx.querySelector('option[value=auto]').textContent = 'auto (' + S.r.name + ')';
      S.busy = !ok;
    });
  });
  ui.pause.addEventListener('click', togglePause);
  ui.hudBtn.addEventListener('click', toggleHud);
  window.addEventListener('resize', sizeCanvas);

  // Start when the demo comes into view; idle while it is scrolled away.
  var io = new IntersectionObserver(function (entries) {
    entries.forEach(function (e) {
      S.visible = e.isIntersecting;
      if (e.isIntersecting) start();
    });
  }, { rootMargin: '200px' });
  io.observe(ui.stage);
})();
