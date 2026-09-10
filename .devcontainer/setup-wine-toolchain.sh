#!/usr/bin/env bash
# Creates Wine wrappers for the official WCH riscv-none-embed-gcc 8.2.0
# toolchain (Windows .exe): one shell wrapper per .exe in /usr/local/bin so
# CMake finds riscv-none-embed-gcc (and binutils) in PATH - same scheme as
# (.github/workflows/build.yml, "Setup RISC-V toolchain env (Wine wrappers)").
# The toolchain itself is bind-mounted from ../dependencies and must already
# be materialized on the host (git lfs pull there, we don't touch git in here).
set -euo pipefail

TOOL_ROOT="/workspaces/esc-firmware/../dependencies/tools/risc/riscv-toolchain/risc-none-embed-gcc-8.2.0"
WRAPPER_DIR="/usr/local/bin"

if [ ! -d "$TOOL_ROOT/bin" ]; then
    echo "WARNING: WCH toolchain not found at $TOOL_ROOT/bin" >&2
    echo "Run on the host:  cd dependencies && git lfs pull --include='tools/risc/**'" >&2
    exit 0
fi

for exe in "$TOOL_ROOT/bin/"*.exe; do
    name=$(basename "${exe%.exe}")
    printf '#!/bin/sh\nexec wine "%s" "$@"\n' "$exe" | sudo tee "/usr/local/bin/$name" >/dev/null
    sudo chmod +x "/usr/local/bin/$name"
done

# mark the real toolchain exes executable (wine can be picky)
chmod +x "$TOOL_ROOT/bin/"*.exe 2>/dev/null || true

# gcc.cmake "Wine mode" hook: point RISCV_WRAPPER_DIR at the wrapper dir
# (set as a CMake cache var in CMakePresets.json)

echo "WCH Wine toolchain wrappers installed:"
ls -la /usr/local/bin/riscv-none-embed-gcc
