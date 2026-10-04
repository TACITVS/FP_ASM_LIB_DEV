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
# Targets / diagnostics (see README "Build targets"):
#   make ISA=x86-64-v3   # portable AVX2 build — runs on any Haswell/Zen1+ CPU
#   make ISA=raptorlake  # tuned for 12th-14th gen Core (AVX2+AVX-VNNI)
#   make ISA=x86-64-v4   # AVX-512 build (Ice Lake, Sapphire Rapids, Zen 4+)
#   make DISPATCH=0      # no runtime kernel dispatch (pure legacy AVX2 symbols)
#   make all-isas        # one library per ISA in build/<isa>/
#   make info            # toolchain + configuration
#   make diag            # CPU/OS/build/dispatch report (build/fpasm-info)
#   make bench-isa       # time every kernel variant this CPU supports
#   make lint-asm        # static check: callee-saved GPR / Win64 xmm6-15 misuse
#   make test-win64-abi  # run the ABI canary test with the Win64 xmm saves forced on
#
# Requirements: NASM (>=2.13) and a C11 compiler (gcc/clang). The kernels
# require a CPU with AVX2 + FMA; faster AVX-VNNI / AVX-512 variants are
# picked at runtime when the CPU has them.
# =============================================================================

LIB        := fpasm
SRC_ASM    := src/asm
SRC_DIRS   := src/wrappers src/algorithms src/runtime
INCLUDE    := include
BUILD      ?= build
OBJ        := $(BUILD)/obj
PREFIX     ?= /usr/local

# --- Platform / toolchain detection ------------------------------------------
UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
ASM      ?= nasm
CC       ?= cc
AR       ?= ar
# --- ISA target --------------------------------------------------------------
# ISA picks the -march for the C code (the assembly is the same for every
# target; its faster variants are chosen at runtime). Presets:
#   native      whatever this machine supports (default; NOT portable)
#   x86-64-v3   portable AVX2+FMA+BMI2 — Haswell / Zen 1 and newer
#   haswell     the original development target (i7-4600M)
#   alderlake   12th gen Core; also fine for 13th/14th gen (same ISA)
#   raptorlake  13th/14th gen Core (falls back to alderlake on older compilers)
#   x86-64-v4   AVX-512 F/BW/CD/DQ/VL (aliases: avx512)
#   anything else is passed through as -march=<ISA> (e.g. znver4, sapphirerapids)
# ARCH=<march> (the old knob) still overrides the -march directly.
ISA      ?= native
cc_has_march = $(shell $(CC) -march=$(1) -E -x c /dev/null -o /dev/null >/dev/null 2>&1 && echo yes)
ifeq ($(ISA),avx512)
    MARCH := x86-64-v4
else ifeq ($(ISA),raptorlake)
    MARCH := $(if $(call cc_has_march,raptorlake),raptorlake,alderlake)
else
    MARCH := $(ISA)
endif
ARCH     ?= $(MARCH)
ifeq (,$(filter clean info,$(MAKECMDGOALS)))
  ifneq ($(call cc_has_march,$(ARCH)),yes)
    $(error $(CC) does not support -march=$(ARCH) (ISA=$(ISA)). Run `make info` for the presets this compiler supports, or upgrade the compiler (raptorlake needs gcc >= 13 / clang >= 16))
  endif
endif

# Runtime kernel dispatch (AVX2 -> AVX-VNNI -> AVX-512). DISPATCH=0 restores
# the legacy layout where the public symbols are the AVX2 kernels themselves.
DISPATCH ?= 1
ifeq ($(DISPATCH),1)
    DISPATCH_DEF := -DFP_DISPATCH
endif

CFLAGS   ?= -O3 -std=c11 -Wall -Wextra
ARCHFLAGS      := -march=$(ARCH)
# CPU detection / dispatch / diagnostics must run on ANY x86-64 so they can
# explain a mismatch instead of crashing: compile them for the baseline.
BASE_ARCHFLAGS := -march=x86-64 -mtune=generic
ALL_CFLAGS = $(CFLAGS) -I$(INCLUDE) -fPIC $(ARCHFLAGS) $(DISPATCH_DEF) \
             -DFPASM_ISA_NAME=$(ISA) -DFPASM_MARCH=$(ARCH)
ASMINC   := -I$(SRC_ASM)/
ASMFLAGS ?=
ALL_ASMFLAGS = $(ASMINC) $(DISPATCH_DEF) $(ASMFLAGS)

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

# Objects compiled for the x86-64 baseline (fp_build_info.c is deliberately
# NOT in this list: it records the real build flags).
# (`private` keeps make from propagating the override to prerequisites.)
$(OBJ)/fp_cpu.o $(OBJ)/fp_dispatch.o: private ARCHFLAGS := $(BASE_ARCHFLAGS)

# Rebuild everything when the configuration changes (switching ISA= without
# `make clean` would otherwise silently mix objects from two targets).
CONFIG_STAMP := $(BUILD)/config.stamp
CONFIG_STR   := ISA=$(ISA) ARCH=$(ARCH) DISPATCH=$(DISPATCH) CC=$(CC) CFLAGS=$(CFLAGS) ASMFLAGS=$(ASMFLAGS)
ifeq (,$(filter clean info,$(MAKECMDGOALS)))
$(shell mkdir -p $(BUILD); [ "`cat $(CONFIG_STAMP) 2>/dev/null`" = "$(CONFIG_STR)" ] || echo "$(CONFIG_STR)" > $(CONFIG_STAMP))
endif
$(OBJS): $(CONFIG_STAMP)

# --- Targets -----------------------------------------------------------------
.PHONY: all static shared test bench showcase clean install dirs \
        info diag check-cpu bench-isa all-isas lint-asm test-win64-abi
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
	$(ASM) -f $(ASMFMT) $(ALL_ASMFLAGS) $< -o $@
	@echo "  ASM  $<"

$(OBJ)/%.o: %.c | dirs
	$(CC) $(ALL_CFLAGS) -c $< -o $@
	@echo "  CC   $<"

# --- Tests: link each tests/test_*.c against the static library --------------
# check-cpu runs first so an ISA/CPU mismatch is a readable error, not SIGILL.
test: $(STATIC) $(TEST_BINS) check-cpu
	@echo "== running tests =="; \
	fail=0; for t in $(TEST_BINS); do \
	    echo "-- $$t --"; $$t || fail=1; \
	done; \
	if [ $$fail -eq 0 ]; then echo "== ALL TEST BINARIES PASSED =="; else echo "== SOME TESTS FAILED =="; exit 1; fi

$(BUILD)/test_%: tests/test_%.c $(STATIC) | dirs
	$(CC) $(ALL_CFLAGS) $< $(STATIC) -o $@ $(LDLIBS)

BENCH_SRCS := $(wildcard benchmarks/bench_*.c)
BENCH_BINS := $(patsubst benchmarks/%.c,$(BUILD)/%,$(BENCH_SRCS))

# Benchmarks are compiled at -O3 -march=native so the scalar reference is
# autovectorized too (a fair comparison against the hand-written kernels).
bench: $(STATIC) $(BENCH_BINS)
	@for b in $(BENCH_BINS); do echo "== $$b =="; $$b >/dev/null; done

$(BUILD)/bench_%: benchmarks/bench_%.c $(STATIC) | dirs
	$(CC) -I$(INCLUDE) -O3 -march=native $< $(STATIC) -o $@ $(LDLIBS)

# Showcases: real algorithms written imperatively vs. composed from the library,
# verified equal and timed. Compiled -O3 -march=native so the imperative
# baseline is as fast as the compiler can make it.
SHOWCASE_SRCS := $(wildcard showcases/showcase_*.c)
SHOWCASE_BINS := $(patsubst showcases/%.c,$(BUILD)/%,$(SHOWCASE_SRCS))
showcase: $(STATIC) $(SHOWCASE_BINS)
	@fail=0; for s in $(SHOWCASE_BINS); do echo "== $$s =="; $$s || fail=1; echo; done; \
	if [ $$fail -eq 0 ]; then echo "== ALL SHOWCASES VERIFIED =="; else echo "== SHOWCASE MISMATCH =="; exit 1; fi

$(BUILD)/showcase_%: showcases/showcase_%.c $(STATIC) | dirs
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

# --- Diagnostics ----------------------------------------------------------------
# fpasm-info: compiled for the x86-64 baseline so it always runs.
$(BUILD)/fpasm-info: tools/fpasm_info.c $(STATIC) | dirs
	$(CC) $(CFLAGS) -I$(INCLUDE) $(BASE_ARCHFLAGS) $< $(STATIC) -o $@ $(LDLIBS)

diag: $(BUILD)/fpasm-info
	@$(BUILD)/fpasm-info

check-cpu: $(BUILD)/fpasm-info
	@$(BUILD)/fpasm-info --check

bench-isa: $(BUILD)/fpasm-info
	@$(BUILD)/fpasm-info --bench

lint-asm:
	@python3 tools/asm_abi_lint.py

# The Win64 ABI keeps xmm6-15 callee-saved; force those saves on in a Linux
# build and verify them with the canary test.
test-win64-abi:
	@$(MAKE) --no-print-directory BUILD=$(BUILD)/win64-abi ASMFLAGS=-DFP_FORCE_XMM_SAVE \
	    ISA=$(ISA) DISPATCH=$(DISPATCH) $(BUILD)/win64-abi/test_abi_preserve
	@FP_TEST_CHECK_XMM=1 $(BUILD)/win64-abi/test_abi_preserve | tail -n 1

# Build one library per ISA side by side: build/<isa>/libfpasm.{a,so}.
ISAS ?= x86-64-v3 raptorlake x86-64-v4
all-isas:
	@for i in $(ISAS); do \
	    echo "== ISA=$$i =="; \
	    $(MAKE) --no-print-directory ISA=$$i BUILD=$(BUILD)/$$i static shared || exit 1; \
	done

info:
	@echo "platform   : $(UNAME_S)  asm-format: $(ASMFMT)  shlib: .$(SHLIB_EXT)"
	@echo "cc         : $(CC) — `$(CC) --version 2>/dev/null | head -n1`"
	@echo "nasm       : `$(ASM) -v 2>/dev/null || echo NOT FOUND`"
	@echo "ISA preset : $(ISA)   ->  -march=$(ARCH)"
	@echo "native is  : `$(CC) -march=native -Q --help=target 2>/dev/null | awk '$$1==\"-march=\"{print $$2; exit}'` (what -march=native resolves to on this machine)"
	@echo "dispatch   : $(if $(DISPATCH_DEF),on (AVX2 -> AVX-VNNI -> AVX-512 at runtime),off (legacy AVX2 symbols))"
	@echo "CFLAGS     : $(ALL_CFLAGS)"
	@echo "runtime/   : $(BASE_ARCHFLAGS) (fp_cpu.c, fp_dispatch.c, fpasm-info)"
	@echo "presets    :$(foreach m,x86-64-v3 haswell alderlake raptorlake x86-64-v4 znver4, $(m)=$(if $(call cc_has_march,$(m)),ok,UNSUPPORTED))"
	@echo "asm srcs   : $(words $(ASM_SRCS))   c srcs: $(words $(C_SRCS))"
	@echo "tests      : $(TEST_SRCS)"
