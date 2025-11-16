#!/bin/bash
echo "=== TOOL AVAILABILITY CHECK ==="
echo

tools=(
    "bash:Bash shell"
    "gcc:C compiler"
    "nasm:NASM assembler"
    "git:Version control"
    "make:Build system"
    "grep:Text search"
    "sed:Stream editor"
    "awk:Text processing"
    "find:File search"
    "python:Python interpreter"
    "python3:Python 3"
    "curl:HTTP client"
    "wget:HTTP downloader"
)

for tool_info in "${tools[@]}"; do
    IFS=':' read -r tool desc <<< "$tool_info"
    if command -v "$tool" &> /dev/null; then
        version=$(command "$tool" --version 2>&1 | head -1)
        echo "✓ $desc ($tool): $version"
    else
        echo "✗ $desc ($tool): NOT FOUND"
    fi
done

echo
echo "=== ENVIRONMENT INFO ==="
echo "Shell: $SHELL"
echo "PATH: $PATH"
echo "PWD: $PWD"
uname -a 2>/dev/null || echo "uname not available (Windows native)"
