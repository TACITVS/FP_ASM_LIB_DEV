# Linux Build Assessment for FP_ASM_LIB_DEV

## Environment
- OS: Ubuntu Linux (containerized)
- Toolchain: GCC 13.3.0, NASM (system provided), CMake 3.27+

## Procedure
1. Configure the project with CMake using a separate build directory:
   ```sh
   cmake -S . -B build/linux
   ```
2. Build all targets, including the NASM object library and static archive:
   ```sh
   cmake --build build/linux
   ```

## Result
- All NASM sources under `src/asm/` assembled successfully using the `elf64` object format.
- C wrapper sources under `src/wrappers/` compiled without errors.
- The static library `libfp_asm.a` linked successfully.
- As expected on a non-Windows platform, unit tests and benchmarks were skipped because they depend on Win64 calling conventions.

## Notes
- The generated static library uses the Win64 ABI semantics defined in the NASM modules. While it links correctly on Linux, executing the higher-level tests that rely on Win64 calling conventions is not supported in this environment.
- Ensure NASM is installed before configuring the build. If `nasm` is missing, install it via your distribution's package manager (e.g., `apt-get install nasm`).
