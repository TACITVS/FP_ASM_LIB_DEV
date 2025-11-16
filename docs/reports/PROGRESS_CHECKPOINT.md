## Checkpoint 1: Initial Assessment and Plan

**Objective:** Build an OpenGL demo showcasing 3D game engine algorithms, using `FP_ASM_LIB` as a separate library.

**Current Status:** User expressed concern about the state of `FP_ASM_LIB` from a previous session. No specific "todo list" provided yet, but the general goal is clear.

**Exploration Results:**
Found numerous existing OpenGL-related demo `.c` files and their corresponding `.bat` build scripts.
Key files identified for further investigation:
*   `demo_fp_graphics_showcase.c` - This file is a raytracer that uses FP_ASM_LIB functions for lighting and post-processing, outputting a PPM image.
*   `Makefile` - Comprehensive for building core library components (assembly and C wrappers) but lacks a specific rule for `demo_fp_graphics_showcase.c`.
*   `build_fp_cube.bat` - Provides a template for building a standalone demo that links against FP_ASM_LIB components.

**Dependencies of `demo_fp_graphics_showcase.c`:**
*   `fp_core.h`
*   `src/engine/fp_graphics_postprocess.c` (included directly)
*   FP_ASM_LIB functions: `fp_fold_dotp_f32`, `fp_map_scale_f32`, `fp_zip_add_f32`, `fp_postprocess_bright_pass`, `fp_postprocess_tonemap_reinhard`, `fp_postprocess_gamma`. These functions are expected to be provided by the compiled FP_ASM_LIB.

**Plan:**
1.  **Build FP_ASM_LIB components:** Run `make asm wrappers` to ensure all necessary `.o` files for the FP_ASM_LIB are generated in `build/obj`.
2.  **Identify required object files:** List the contents of `build/obj` to determine which `.o` files are needed for linking.
3.  **Create `build_fp_graphics_showcase.bat`:** Develop a new batch script based on `build_fp_cube.bat` to compile and link `demo_fp_graphics_showcase.c` with the FP_ASM_LIB object files and necessary system libraries.
4.  **Attempt to build and run:** Execute the new batch script to build `fp_graphics_showcase.exe` and verify its execution.
5.  **Propose Refinement/New Demo:** Based on the investigation, propose whether to refine an existing showcase demo or create a new one to best meet the user's objective.