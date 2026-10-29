#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Optional user-mode runner when the host kernel cannot execute i386 ELF files.
set -eu
ps3me_qemu_binary="${PS3ME_QEMU_I386:-qemu-i386}"
if [ -n "${PS3ME_I386_SYSROOT:-}" ]; then
    set -- -L "$PS3ME_I386_SYSROOT" "$@"
fi
if [ -n "${PS3ME_I386_LIBRARY_PATH:-}" ]; then
    set -- -E "LD_LIBRARY_PATH=$PS3ME_I386_LIBRARY_PATH" "$@"
fi
exec "$ps3me_qemu_binary" "$@"
