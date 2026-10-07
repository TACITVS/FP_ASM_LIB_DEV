#!/usr/bin/env python3
"""asm_abi_lint.py — static check of callee-saved register use in src/asm.

For every exported function it reports:
  * GPR:  rbx / r12-r15 written but never pushed          (SysV + Win64 bug)
          rsi / rdi written but never pushed                (Win64 bug)
  * XMM:  xmm6-xmm15 / ymm6-15 written but never saved    (Win64 bug)

A function counts as saving a register when it pushes it, stores it to
memory, or uses the PROLOGUE (rbx, r12-r15, ymm6-13) / XMM_SAVE_WIN64
(xmm6-15) macros. It is a heuristic linter: it reads instructions, not
control flow, so treat a report as "look here", and the runtime canary test
(tests/test_abi_preserve.c) as the proof.

Usage: tools/asm_abi_lint.py [src/asm/*.asm]   (exit 1 if anything is found)
"""
import glob
import re
import sys

GPR_ALIASES = {
    "rbx": ("rbx", "ebx", "bx", "bl"),
    "rsi": ("rsi", "esi", "si", "sil"),     # callee-saved on Win64 only
    "rdi": ("rdi", "edi", "di", "dil"),     # callee-saved on Win64 only
    "r12": ("r12", "r12d", "r12w", "r12b"),
    "r13": ("r13", "r13d", "r13w", "r13b"),
    "r14": ("r14", "r14d", "r14w", "r14b"),
    "r15": ("r15", "r15d", "r15w", "r15b"),
}


def functions(path):
    # Code inside `%ifdef FP_SYSV` runs only on SysV, where rsi/rdi carry the
    # arguments and are caller-saved: drop it before looking at registers.
    text = re.sub(r"%ifdef FP_SYSV.*?%endif", "", open(path).read(), flags=re.S)
    exported = set(re.findall(r"global\s+(\w+)", text)) | set(re.findall(r"FP_DISPATCHED\s+(\w+)", text))
    funcs, cur = {}, None
    for raw in text.split("\n"):
        s = raw.split(";")[0].strip()
        m = re.match(r"^(\w+):", s) or re.match(r"^FP_DISPATCHED\s+(\w+)", s)
        if m and m.group(1) in exported:
            cur = m.group(1)
            funcs[cur] = []
            continue
        if s.startswith("%macro"):
            cur = None
        if cur:
            funcs[cur].append(s)
    return funcs


def lint(path):
    problems = []
    for fn, body in functions(path).items():
        text = "\n".join(body)
        gpr_saved = set(re.findall(r"push\s+(\w+)", text))
        xmm_saved = set()
        if "PROLOGUE" in body:
            gpr_saved |= {"rbx", "r12", "r13", "r14", "r15"}
            xmm_saved |= set(range(6, 14))
        if "XMM_SAVE_WIN64" in body:
            xmm_saved |= set(range(6, 16))
        bad_gpr, xmm_used = [], set()
        for reg, names in GPR_ALIASES.items():
            if reg in gpr_saved:
                continue
            if any(re.match(r"^[a-z0-9]+\s+" + n + r"\b", s) for s in body for n in names):
                bad_gpr.append(reg)
        for s in body:
            m = re.match(r"^[a-z0-9]+\s+[xyz]mm(\d+)\b", s)
            if m and 6 <= int(m.group(1)) <= 15:
                xmm_used.add(int(m.group(1)))
            m = re.match(r"^v?mov\w*\s+\[[^\]]*\]\s*,\s*[xy]mm(\d+)", s)
            if m:
                xmm_saved.add(int(m.group(1)))
        bad_xmm = sorted(xmm_used - xmm_saved)
        if bad_gpr:
            problems.append(f"{path}: {fn}: clobbers callee-saved {', '.join(bad_gpr)} "
                            f"({'Win64' if set(bad_gpr) <= {'rsi', 'rdi'} else 'SysV and/or Win64'})")
        if bad_xmm:
            problems.append(f"{path}: {fn}: clobbers xmm{', xmm'.join(map(str, bad_xmm))} "
                            "without saving (Win64 callee-saved; use XMM_SAVE_WIN64)")
    return problems


def main(argv):
    paths = argv[1:] or sorted(glob.glob("src/asm/*.asm"))
    problems = [p for path in paths for p in lint(path)]
    for p in problems:
        print(p)
    print(f"asm-abi-lint: {len(paths)} files, {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
