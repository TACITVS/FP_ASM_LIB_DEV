# FP-ASM Library - Master Makefile
# Professional build system for the reorganized project

# Directories
SRC_ASM = src/asm
SRC_WRAPPERS = src/wrappers
INCLUDE = include
BUILD_OBJ = build/obj
BUILD_BIN = build/bin
TESTS = tests
BENCHMARKS = benchmarks

# Compiler and assembler
ASM = nasm
CC = gcc

ifeq ($(OS),Windows_NT)
ASMFLAGS = -f win64
else
ASMFLAGS = -f elf64 -p $(SRC_ASM)/nonexecstack.inc
endif

CFLAGS = -I$(INCLUDE) -O3 -march=native
LDFLAGS = -lm -Wl,--allow-multiple-definition

# Assembly source files
ASM_SOURCES = $(wildcard $(SRC_ASM)/*.asm)
ASM_OBJECTS = $(patsubst $(SRC_ASM)/%.asm,$(BUILD_OBJ)/%.o,$(ASM_SOURCES))

# Wrapper source files
WRAPPER_SOURCES = $(wildcard $(SRC_WRAPPERS)/*.c)
WRAPPER_OBJECTS = $(patsubst $(SRC_WRAPPERS)/%.c,$(BUILD_OBJ)/%.o,$(WRAPPER_SOURCES))

# Test/benchmark source files (platform-filtered)
TEST_SOURCES = $(wildcard $(TESTS)/*.c)
BENCH_SOURCES = $(wildcard $(BENCHMARKS)/demo_*.c)

ifeq ($(OS),Windows_NT)
    # Windows builds consume the entire matrix
else
    TEST_SOURCES := $(filter-out $(TESTS)/test_zip_timed.c,$(TEST_SOURCES))
    BENCH_SOURCES := $(BENCHMARKS)/demo_bench_3d.c
endif

TEST_BINARIES = $(patsubst $(TESTS)/%.c,$(BUILD_BIN)/%.exe,$(TEST_SOURCES))
BENCH_BINARIES = $(patsubst $(BENCHMARKS)/%.c,$(BUILD_BIN)/%.exe,$(BENCH_SOURCES))

# Default target
.PHONY: all
all: asm wrappers

# Build all assembly modules
.PHONY: asm
asm: | dirs
asm: $(ASM_OBJECTS)

# Build all wrappers
.PHONY: wrappers
wrappers: | dirs
wrappers: $(WRAPPER_OBJECTS)

# Build all tests
.PHONY: tests
tests: | dirs
tests: $(TEST_BINARIES)

# Build all benchmarks
.PHONY: benchmarks
benchmarks: | dirs
benchmarks: $(BENCH_BINARIES)

# Build everything
.PHONY: complete
complete: asm wrappers tests benchmarks

# Assembly compilation rule
$(BUILD_OBJ)/%.o: $(SRC_ASM)/%.asm | dirs
	@echo "Assembling $<..."
	$(ASM) $(ASMFLAGS) $< -o $@

# Wrapper compilation rule
$(BUILD_OBJ)/%.o: $(SRC_WRAPPERS)/%.c | dirs
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Test compilation rule
$(BUILD_BIN)/test_3d_math.exe: tests/test_3d_math.c \
$(BUILD_OBJ)/fp_core_3dmath_f32.o $(BUILD_OBJ)/fp_3d_math.o | dirs
	@echo "Building test $@..."
	$(CC) $(CFLAGS) $< $(BUILD_OBJ)/fp_core_3dmath_f32.o \
$(BUILD_OBJ)/fp_3d_math.o -o $@ $(LDFLAGS)

$(BUILD_BIN)/test_%.exe: $(TESTS)/test_%.c $(ASM_OBJECTS) $(WRAPPER_OBJECTS) | dirs
	@echo "Building test $@..."
	$(CC) $(CFLAGS) $< $(ASM_OBJECTS) $(WRAPPER_OBJECTS) -o $@ $(LDFLAGS)

# Benchmark compilation rule (demo_*.c files)
$(BUILD_BIN)/demo_bench_3d.exe: $(BENCHMARKS)/demo_bench_3d.c \
$(BUILD_OBJ)/fp_core_3dmath_f32.o $(BUILD_OBJ)/fp_3d_math.o | dirs
	@echo "Building benchmark $@..."
	$(CC) $(CFLAGS) $< $(BUILD_OBJ)/fp_core_3dmath_f32.o \
$(BUILD_OBJ)/fp_3d_math.o -o $@ $(LDFLAGS)

$(BUILD_BIN)/demo_%.exe: $(BENCHMARKS)/demo_%.c $(ASM_OBJECTS) $(WRAPPER_OBJECTS) | dirs
	@echo "Building benchmark $@..."
	$(CC) $(CFLAGS) $< $(ASM_OBJECTS) $(WRAPPER_OBJECTS) -o $@ $(LDFLAGS)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(BUILD_OBJ)/*.o
	rm -f $(BUILD_BIN)/*.exe

# Clean everything including directories
.PHONY: distclean
distclean: clean
	@echo "Removing build directories..."
	rm -rf $(BUILD_OBJ) $(BUILD_BIN)

# Recreate build directories
.PHONY: dirs
dirs:
	@echo "Creating build directories..."
	mkdir -p $(BUILD_OBJ) $(BUILD_BIN)

# Show build variables (for debugging)
.PHONY: show
show:
	@echo "ASM_SOURCES: $(ASM_SOURCES)"
	@echo "ASM_OBJECTS: $(ASM_OBJECTS)"
	@echo "WRAPPER_SOURCES: $(WRAPPER_SOURCES)"
	@echo "WRAPPER_OBJECTS: $(WRAPPER_OBJECTS)"
	@echo "TEST_SOURCES: $(TEST_SOURCES)"
	@echo "TEST_BINARIES: $(TEST_BINARIES)"
	@echo "BENCH_SOURCES: $(BENCH_SOURCES)"
	@echo "BENCH_BINARIES: $(BENCH_BINARIES)"

# Help target
.PHONY: help
help:
	@echo "FP-ASM Library Build System"
	@echo "============================"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build assembly modules and wrappers (default)"
	@echo "  asm        - Build all assembly modules"
	@echo "  wrappers   - Build all C wrappers"
	@echo "  tests      - Build all test executables"
	@echo "  benchmarks - Build all benchmark executables"
	@echo "  complete   - Build everything (asm + wrappers + tests + benchmarks)"
	@echo "  clean      - Remove object files and executables"
	@echo "  distclean  - Remove all build artifacts and directories"
	@echo "  dirs       - Create build directories"
	@echo "  show       - Show build variables"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build core library"
	@echo "  make complete     # Build everything"
	@echo "  make tests        # Build only tests"
	@echo "  make clean        # Clean build artifacts"
