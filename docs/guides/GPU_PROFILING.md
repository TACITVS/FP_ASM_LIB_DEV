# GPU Performance Profiling - Finding the Bottleneck

## Problem Statement

Current GPU performance is **27x slower** than theoretical maximum:
- **Actual**: 6.67 FPS @ 640×480 (150ms per frame)
- **Expected**: ~180 FPS @ 640×480 (~5.5ms per frame)

You correctly identified this as an **algorithmic problem**. Let's prove it.

---

## Hypothesis

The current implementation is doing WAY too much work per frame:

**Every single frame, we:**
1. ✅ Initialize OpenCL (get platform, device, create context, queue)
2. ✅ **Read kernel file from disk** (`fopen`, `fread`, `fclose`)
3. ✅ **Compile kernel** (OpenCL JIT compilation)
4. ✅ Create GPU buffers
5. ✅ Transfer scene data to GPU
6. ✅ Execute kernel (ONLY THIS SHOULD HAPPEN!)
7. ✅ Read back framebuffer
8. ✅ **Destroy everything** (cleanup all resources)

**Steps 1-5 and 8 should only happen ONCE, not every frame!**

---

## Testing the Hypothesis

Run this command to see the performance breakdown:

```batch
build_gpu_profiled.bat
```

This will print detailed timing for each phase:

```
=== GPU Performance Breakdown ===
Initialization (platform, context, kernel compile): ??? ms
Buffer creation + data transfer:                   ??? ms
Kernel execution:                                   ??? ms
Readback (GPU -> CPU):                              ??? ms
Total GPU time:                                     ??? ms
=================================
```

---

## Expected Results

### For 640×480 (150ms total):

My prediction:
- **Initialization: ~80-100ms** ← MASSIVE WASTE! (kernel compilation from disk)
- **Buffer creation: ~10-20ms** ← Recreating buffers unnecessarily
- **Kernel execution: ~5-10ms** ← Actual GPU work (should be ONLY part)
- **Readback: ~5-10ms** ← Transferring pixels back
- **Cleanup: ~10-20ms** ← Destroying everything we just created

**The kernel execution is probably fast**, but we're drowning it in overhead!

---

## Why This Is Insane

For real-time rendering at 60 FPS, we have a **16.67ms frame budget**.

Currently we're spending:
- **~100ms** compiling a kernel we already compiled last frame
- **~20ms** creating buffers that haven't changed
- **~10ms** actually rendering
- **~20ms** destroying everything so we can do it all again

**This is like:**
- Disassembling your car after every drive
- Recompiling your code for every function call
- Reinstalling Windows before every boot

---

## The Fix (Coming Next)

Once profiling confirms this, we'll implement **persistent GPU state**:

### Initialization (ONCE at startup):
```c
gpu_context = gpu_init();  // Platform, device, context, queue, kernel
gpu_scene = gpu_upload_scene(scene);  // Spheres, planes, lights (ONCE)
```

### Per-frame rendering (60 FPS):
```c
gpu_render_frame(gpu_context, gpu_scene, camera, framebuffer);
// Only kernel execution + readback (~15ms total)
```

### Cleanup (ONCE at shutdown):
```c
gpu_cleanup(gpu_context, gpu_scene);
```

---

## Expected Performance After Fix

| Phase | Current (ms) | After Fix (ms) | Speedup |
|-------|-------------|----------------|---------|
| Initialization | 100 | 0 (amortized) | ∞ |
| Buffer creation | 20 | 0 (persistent) | ∞ |
| **Kernel execution** | **10** | **10** | **1x** |
| Readback | 10 | 10 | 1x |
| Cleanup | 20 | 0 (amortized) | ∞ |
| **TOTAL** | **160ms** | **~20ms** | **8x** |

**Expected FPS:**
- 640×480: **50+ FPS** (currently 6.67 FPS) - 8x improvement
- 1920×1080: **30-40 FPS** (currently 9.26 FPS) - 3-4x improvement

This would put us in the **real-time range** you experienced with OpenGL!

---

## Next Steps

1. **Run profiling**: `build_gpu_profiled.bat`
2. **Paste output** here so we can see the breakdown
3. **Implement persistent state** API
4. **Achieve real-time performance** (30-60 FPS @ 1080p)

---

## Your OpenGL Experience

You mentioned having a game engine with physics running in real-time using OpenGL/GLEW.

**OpenGL rendering loop structure:**
```c
// Initialization (ONCE)
GLuint shader = compile_shader(...);
GLuint vbo = create_buffer(...);
upload_geometry(vbo, vertices);

// Per-frame (60 FPS)
while (running) {
    glUseProgram(shader);
    glBindBuffer(vbo);
    glDrawArrays(...);  // <-- ONLY THIS PER FRAME
}

// Cleanup (ONCE)
glDeleteBuffers(vbo);
glDeleteProgram(shader);
```

**Our current broken approach is equivalent to:**
```c
// Every frame (INSANE!)
while (running) {
    GLuint shader = compile_shader(...);  // ← RECOMPILE EVERY FRAME?!
    GLuint vbo = create_buffer(...);      // ← RECREATE EVERY FRAME?!
    upload_geometry(vbo, vertices);       // ← REUPLOAD EVERY FRAME?!
    glDrawArrays(...);
    glDeleteBuffers(vbo);                 // ← DELETE EVERY FRAME?!
    glDeleteProgram(shader);              // ← DELETE EVERY FRAME?!
}
```

**No wonder it's slow!**

---

## TL;DR

Run `build_gpu_profiled.bat` and paste the output. The profiling will prove that we're wasting 90% of our time on per-frame initialization that should only happen once.

Then we'll fix it and achieve the real-time performance you're expecting. 🚀
