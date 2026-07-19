# =============================================================================
# FP-ASM — portable build system
#
# Produces a linkable library (static + shared) from the x64 AVX2 assembly
# kernels and their C wrappers, for consumption by games / graphics projects.
#
#   make            # build static + shared libraries into build/
#   make static     # build build/libfpasm.a
#   make shared     # build build/libfpasm.so (.dll on Windows)
#   make test       # build and run the test suite
#   make install    # install headers + libs under PREFIX (default /usr/local)
#   make clean
#
# Requirements: NASM (>=2.13) and a C11 compiler (gcc/clang). The kernels
# require a CPU with AVX2 + FMA.
# =============================================================================

LIB        := fpasm
SRC_ASM    := src/asm
SRC_DIRS   := src/wrappers src/algorithms
INCLUDE    := include
BUILD      := build
OBJ        := $(BUILD)/obj
PREFIX     ?= /usr/local

# --- Platform / toolchain detection ------------------------------------------
UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
ASM      ?= nasm
CC       ?= cc
AR       ?= ar
# CPU features: the ASM needs AVX2; override ARCH= to retarget the C code.
ARCH     ?= native
CFLAGS   ?= -O3 -std=c11 -Wall -Wextra
CFLAGS   += -I$(INCLUDE) -march=$(ARCH) -fPIC
ASMINC   := -I$(SRC_ASM)/

ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(findstring CYGWIN,$(UNAME_S)))
    ASMFMT     := win64
    SHLIB_EXT  := dll
    LDLIBS     :=
else ifeq ($(UNAME_S),Darwin)
    ASMFMT     := macho64
    SHLIB_EXT  := dylib
    LDLIBS     := -lm
else
    ASMFMT     := elf64
    SHLIB_EXT  := so
    LDLIBS     := -lm
endif

STATIC := $(BUILD)/lib$(LIB).a
SHARED := $(BUILD)/lib$(LIB).$(SHLIB_EXT)

# --- Sources / objects -------------------------------------------------------
ASM_SRCS := $(wildcard $(SRC_ASM)/*.asm)
C_SRCS   := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.c))
ASM_OBJS := $(patsubst %.asm,$(OBJ)/%.o,$(notdir $(ASM_SRCS)))
C_OBJS   := $(patsubst %.c,$(OBJ)/%.o,$(notdir $(C_SRCS)))
OBJS     := $(ASM_OBJS) $(C_OBJS)

# Let make find sources in their subdirectories.
vpath %.asm $(SRC_ASM)
vpath %.c $(SRC_DIRS)

TEST_SRCS := $(wildcard tests/test_*.c)
TEST_BINS := $(patsubst tests/%.c,$(BUILD)/%,$(TEST_SRCS))

# --- Targets -----------------------------------------------------------------
.PHONY: all static shared test bench clean install dirs
all: static shared

static: $(STATIC)
shared: $(SHARED)

$(STATIC): $(OBJS) | dirs
	$(AR) rcs $@ $(OBJS)
	@echo "  AR   $@"

$(SHARED): $(OBJS) | dirs
	$(CC) -shared -o $@ $(OBJS) $(LDLIBS)
	@echo "  LD   $@"

$(OBJ)/%.o: %.asm | dirs
	$(ASM) -f $(ASMFMT) $(ASMINC) $< -o $@
	@echo "  ASM  $<"

$(OBJ)/%.o: %.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC   $<"

# --- Tests: link each tests/test_*.c against the static library --------------
test: $(STATIC) $(TEST_BINS)
	@echo "== running tests =="; \
	fail=0; for t in $(TEST_BINS); do \
	    echo "-- $$t --"; $$t || fail=1; \
	done; \
	if [ $$fail -eq 0 ]; then echo "== ALL TEST BINARIES PASSED =="; else echo "== SOME TESTS FAILED =="; exit 1; fi

$(BUILD)/%: tests/%.c $(STATIC) | dirs
	$(CC) $(CFLAGS) $< $(STATIC) -o $@ $(LDLIBS)

BENCH_SRCS := $(wildcard benchmarks/bench_*.c)
BENCH_BINS := $(patsubst benchmarks/%.c,$(BUILD)/%,$(BENCH_SRCS))

# Benchmarks are compiled at -O3 -march=native so the scalar reference is
# autovectorized too (a fair comparison against the hand-written kernels).
bench: $(STATIC) $(BENCH_BINS)
	@for b in $(BENCH_BINS); do echo "== $$b =="; $$b >/dev/null; done

$(BUILD)/bench_%: benchmarks/bench_%.c $(STATIC) | dirs
	$(CC) -I$(INCLUDE) -O3 -march=native $< $(STATIC) -o $@ $(LDLIBS)

dirs:
	@mkdir -p $(OBJ)

install: all
	@mkdir -p $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/$(LIB)
	cp $(STATIC) $(SHARED) $(DESTDIR)$(PREFIX)/lib/
	cp $(INCLUDE)/*.h $(DESTDIR)$(PREFIX)/include/$(LIB)/
	@echo "installed to $(DESTDIR)$(PREFIX)"

clean:
	rm -rf $(BUILD)

# Diagnostics
.PHONY: info
info:
	@echo "platform : $(UNAME_S)  asm-format: $(ASMFMT)  shlib: .$(SHLIB_EXT)"
	@echo "asm srcs : $(words $(ASM_SRCS))   c srcs: $(words $(C_SRCS))"
	@echo "tests    : $(TEST_SRCS)"
