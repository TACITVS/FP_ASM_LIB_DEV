# Build Scripts and Tools

This directory contains all build scripts and development tools for the FP-ASM library.

## Directory Structure

```
scripts/
├── build/           # Build scripts for tests, benchmarks, and examples
├── test/            # Test runner scripts
└── tools/           # Development and maintenance tools
```

## Quick Start

### Build All Tests
```bash
cd scripts/build
build_all_tests.bat
```

### Build Specific Component
```bash
cd scripts/build
build_test_critical.bat          # Critical bug fix tests
build_test_u64_comprehensive.bat  # U64 comprehensive tests
```

## Build Scripts Organization

All build scripts are located in `scripts/build/` and follow this naming convention:

- `build_test_*.bat` - Test suite build scripts
- `build_bench_*.bat` - Benchmark build scripts  
- `build_*_demo.bat` - Demo/example build scripts

## Tools

Development tools are located in `scripts/tools/`:

- `check_abi.sh` - Check Windows x64 ABI compliance
- `check_registers.sh` - Verify register usage
- `verify_epilogues.sh` - Validate function epilogues
- `generate_html_docs.py` - Generate HTML documentation

## Output Locations

- **Object files**: `../../build/obj/`
- **Executables**: `../../build/bin/`
- **Libraries**: `../../build/lib/`

## Notes

- All scripts expect to be run from within the `scripts/build/` directory
- Paths are relative to the script location (use `../../` to reach project root)
- Executables are output to `build/bin/` for clean organization

