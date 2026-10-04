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
#   make GFX=d3d11       # default renderer conventions (fp_gfx_default()): Direct3D 11
#   make all-isas        # one library per ISA in build/<isa>/
#   make info            # toolchain + configuration
#   make diag            # CPU/OS/build/dispatch report (build/fpasm-info)
#   make bench-isa       # time every kernel variant this CPU supports
#   make lint-asm        # static check: callee-saved GPR / Win64 xmm6-15 misuse
#   make TARGET_OS=windows run-example-d3d11   # Direct3D 11 end-to-end smoke test
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
# TARGET_OS defaults to the build machine (MSYS2/MinGW -> windows). Set it to
# cross-compile, e.g. from Linux/WSL2 for Windows:
#   make TARGET_OS=windows CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar \
#        ISA=x86-64-v3 RUN=wine64 test
# RUN is a prefix for executing built programs (tests, fpasm-info).
UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(findstring CYGWIN,$(UNAME_S)))
    HOST_OS := windows
else ifeq ($(UNAME_S),Darwin)
    HOST_OS := macos
else
    HOST_OS := linux
endif
TARGET_OS ?= $(HOST_OS)
ASM      ?= nasm
CC       ?= cc
AR       ?= ar
RUN      ?=
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

# Renderer conventions default (include/fp_gfx.h). Only selects which preset
# fp_gfx_default() returns; every preset stays available at runtime and the
# library never links or includes any graphics API.
#   GFX=opengl (default) | d3d11 | d3d11-lh | d3d12 | vulkan | metal
GFX      ?= opengl
GFX_ENUM_opengl   := FP_GFX_OPENGL
GFX_ENUM_generic  := FP_GFX_OPENGL
GFX_ENUM_d3d11    := FP_GFX_D3D11
GFX_ENUM_dx11     := FP_GFX_D3D11
GFX_ENUM_d3d11-lh := FP_GFX_D3D11_LH
GFX_ENUM_d3d12    := FP_GFX_D3D12
GFX_ENUM_vulkan   := FP_GFX_VULKAN
GFX_ENUM_metal    := FP_GFX_METAL
GFX_ENUM := $(GFX_ENUM_$(GFX))
ifeq (,$(GFX_ENUM))
  $(error Unknown GFX=$(GFX). Use opengl, d3d11, d3d11-lh, d3d12, vulkan or metal)
endif

CFLAGS   ?= -O3 -std=c11 -Wall -Wextra
ARCHFLAGS      := -march=$(ARCH)
# CPU detection / dispatch / diagnostics must run on ANY x86-64 so they can
# explain a mismatch instead of crashing: compile them for the baseline.
BASE_ARCHFLAGS := -march=x86-64 -mtune=generic
ALL_CFLAGS = $(CFLAGS) -I$(INCLUDE) $(PICFLAG) $(ARCHFLAGS) $(DISPATCH_DEF) \
             -DFPASM_ISA_NAME=$(ISA) -DFPASM_MARCH=$(ARCH) \
             -DFPASM_GFX_DEFAULT=$(GFX_ENUM) -DFPASM_GFX_NAME=$(GFX)
ASMINC   := -I$(SRC_ASM)/
ASMFLAGS ?=
ALL_ASMFLAGS = $(ASMINC) $(DISPATCH_DEF) $(ASMFLAGS)

ifeq ($(TARGET_OS),windows)
    ASMFMT     := win64
    SHLIB_EXT  := dll
    EXE        := .exe
    PICFLAG    :=
    LDLIBS     :=
    # MSVC / D3D projects link the DLL through this import library.
    SHLIB_LDFLAGS := -Wl,--out-implib,$(BUILD)/lib$(LIB).dll.a
else ifeq ($(TARGET_OS),macos)
    ASMFMT     := macho64
    SHLIB_EXT  := dylib
    EXE        :=
    PICFLAG    := -fPIC
    LDLIBS     := -lm
else
    ASMFMT     := elf64
    SHLIB_EXT  := so
    EXE        :=
    PICFLAG    := -fPIC
    LDLIBS     := -lm
endif
INFO   := $(BUILD)/fpasm-info$(EXE)

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
TEST_BINS := $(patsubst tests/%.c,$(BUILD)/%$(EXE),$(TEST_SRCS))

# Objects compiled for the x86-64 baseline (fp_build_info.c is deliberately
# NOT in this list: it records the real build flags).
# (`private` keeps make from propagating the override to prerequisites.)
$(OBJ)/fp_cpu.o $(OBJ)/fp_dispatch.o: private ARCHFLAGS := $(BASE_ARCHFLAGS)

# Rebuild everything when the configuration changes (switching ISA= without
# `make clean` would otherwise silently mix objects from two targets).
CONFIG_STAMP := $(BUILD)/config.stamp
# For -march=native, record what "native" means on this machine, so moving
# the tree to another CPU (or a cloud VM landing on a different host) rebuilds.
ifeq ($(ARCH),native)
  NATIVE_MARCH := $(shell $(CC) -march=native -Q --help=target 2>/dev/null | awk '$$1=="-march="{print $$2; exit}')
  NATIVE_SIG   := $(NATIVE_MARCH)/$(shell grep -m1 -o -w 'flags.*' /proc/cpuinfo 2>/dev/null | cksum | cut -d' ' -f1)
endif
CONFIG_STR   := TARGET_OS=$(TARGET_OS) ISA=$(ISA) ARCH=$(ARCH) NATIVE=$(NATIVE_SIG) GFX=$(GFX) DISPATCH=$(DISPATCH) CC=$(CC) CFLAGS=$(CFLAGS) ASMFLAGS=$(ASMFLAGS)
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
	$(CC) -shared -o $@ $(OBJS) $(SHLIB_LDFLAGS) $(LDLIBS)
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
	    echo "-- $$t --"; $(RUN) $$t || fail=1; \
	done; \
	if [ $$fail -eq 0 ]; then echo "== ALL TEST BINARIES PASSED =="; else echo "== SOME TESTS FAILED =="; exit 1; fi

$(BUILD)/test_%$(EXE): tests/test_%.c $(STATIC) | dirs
	$(CC) $(ALL_CFLAGS) $< $(STATIC) -o $@ $(LDLIBS)

BENCH_SRCS := $(wildcard benchmarks/bench_*.c)
BENCH_BINS := $(patsubst benchmarks/%.c,$(BUILD)/%$(EXE),$(BENCH_SRCS))

# Benchmarks are compiled at -O3 -march=native so the scalar reference is
# autovectorized too (a fair comparison against the hand-written kernels).
bench: $(STATIC) $(BENCH_BINS)
	@for b in $(BENCH_BINS); do echo "== $$b =="; $(RUN) $$b >/dev/null; done

$(BUILD)/bench_%$(EXE): benchmarks/bench_%.c $(STATIC) | dirs
	$(CC) -I$(INCLUDE) -O3 -march=native $< $(STATIC) -o $@ $(LDLIBS)

# Showcases: real algorithms written imperatively vs. composed from the library,
# verified equal and timed. Compiled -O3 -march=native so the imperative
# baseline is as fast as the compiler can make it.
SHOWCASE_SRCS := $(wildcard showcases/showcase_*.c)
SHOWCASE_BINS := $(patsubst showcases/%.c,$(BUILD)/%$(EXE),$(SHOWCASE_SRCS))
showcase: $(STATIC) $(SHOWCASE_BINS)
	@fail=0; for s in $(SHOWCASE_BINS); do echo "== $$s =="; $(RUN) $$s || fail=1; echo; done; \
	if [ $$fail -eq 0 ]; then echo "== ALL SHOWCASES VERIFIED =="; else echo "== SHOWCASE MISMATCH =="; exit 1; fi

$(BUILD)/showcase_%$(EXE): showcases/showcase_%.c $(STATIC) | dirs
	$(CC) -I$(INCLUDE) -O3 -march=native $< $(STATIC) -o $@ $(LDLIBS)

dirs:
	@mkdir -p $(OBJ)

install: all
	@mkdir -p $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/$(LIB)
	cp $(STATIC) $(SHARED) $(wildcard $(BUILD)/lib$(LIB).dll.a) $(DESTDIR)$(PREFIX)/lib/
	cp $(INCLUDE)/*.h $(DESTDIR)$(PREFIX)/include/$(LIB)/
	@mkdir -p $(DESTDIR)$(PREFIX)/share/$(LIB)/shaders
	cp shaders/* $(DESTDIR)$(PREFIX)/share/$(LIB)/shaders/
	@echo "installed to $(DESTDIR)$(PREFIX)"

clean:
	rm -rf $(BUILD)

# --- Diagnostics ----------------------------------------------------------------
# fpasm-info: compiled for the x86-64 baseline so it always runs.
$(INFO): tools/fpasm_info.c $(STATIC) | dirs
	$(CC) $(CFLAGS) -I$(INCLUDE) $(BASE_ARCHFLAGS) $< $(STATIC) -o $@ $(LDLIBS)

diag: $(INFO)
	@$(RUN) $(INFO)

check-cpu: $(INFO)
	@$(RUN) $(INFO) --check

bench-isa: $(INFO)
	@$(RUN) $(INFO) --bench

# --- Renderer examples (consumers of the library; never linked into it) -----
# Direct3D 11 headless smoke test (WARP rasterizer; --hardware for the GPU).
D3D11_EXAMPLE := $(BUILD)/fpasm_d3d11_smoke$(EXE)
.PHONY: example-d3d11 run-example-d3d11
example-d3d11: $(D3D11_EXAMPLE)
$(D3D11_EXAMPLE): examples/d3d11/fpasm_d3d11_smoke.c $(STATIC) | dirs
ifneq ($(TARGET_OS),windows)
	$(error example-d3d11 needs TARGET_OS=windows (MSYS2/MinGW or a mingw-w64 cross compiler))
endif
	$(CC) $(CFLAGS) -I$(INCLUDE) $(BASE_ARCHFLAGS) $< $(STATIC) -o $@ -ld3d11 -ld3dcompiler
run-example-d3d11: $(D3D11_EXAMPLE)
	$(RUN) $(D3D11_EXAMPLE)

lint-asm:
	@python3 tools/asm_abi_lint.py

# The Win64 ABI keeps xmm6-15 callee-saved; force those saves on in a Linux
# build and verify them with the canary test.
test-win64-abi:
	@$(MAKE) --no-print-directory BUILD=$(BUILD)/win64-abi ASMFLAGS=-DFP_FORCE_XMM_SAVE \
	    ISA=$(ISA) DISPATCH=$(DISPATCH) $(BUILD)/win64-abi/test_abi_preserve$(EXE)
	@FP_TEST_CHECK_XMM=1 $(RUN) $(BUILD)/win64-abi/test_abi_preserve$(EXE) | grep -E '^(FAIL|abi:|ALL PASS|SOME FAILED)'

# Build one library per ISA side by side: build/<isa>/libfpasm.{a,so}.
ISAS ?= x86-64-v3 raptorlake x86-64-v4
all-isas:
	@for i in $(ISAS); do \
	    echo "== ISA=$$i =="; \
	    $(MAKE) --no-print-directory ISA=$$i BUILD=$(BUILD)/$$i static shared || exit 1; \
	done

info:
	@echo "platform   : host $(HOST_OS) ($(UNAME_S)) -> target $(TARGET_OS)  asm-format: $(ASMFMT)  shlib: .$(SHLIB_EXT)$(if $(RUN),  run via: $(RUN))"
	@echo "cc         : $(CC) — `$(CC) --version 2>/dev/null | head -n1`"
	@echo "nasm       : `$(ASM) -v 2>/dev/null || echo NOT FOUND`"
	@echo "ISA preset : $(ISA)   ->  -march=$(ARCH)"
	@echo "native is  : `$(CC) -march=native -Q --help=target 2>/dev/null | awk '$$1==\"-march=\"{print $$2; exit}'` (what -march=native resolves to on this machine)"
	@echo "gfx        : $(GFX) (fp_gfx_default(); all presets available at runtime)"
	@echo "dispatch   : $(if $(DISPATCH_DEF),on (AVX2 -> AVX-VNNI -> AVX-512 at runtime),off (legacy AVX2 symbols))"
	@echo "CFLAGS     : $(ALL_CFLAGS)"
	@echo "runtime/   : $(BASE_ARCHFLAGS) (fp_cpu.c, fp_dispatch.c, fpasm-info)"
	@echo "presets    :$(foreach m,x86-64-v3 haswell alderlake raptorlake x86-64-v4 znver4, $(m)=$(if $(call cc_has_march,$(m)),ok,UNSUPPORTED))"
	@echo "asm srcs   : $(words $(ASM_SRCS))   c srcs: $(words $(C_SRCS))"
	@echo "tests      : $(TEST_SRCS)"
