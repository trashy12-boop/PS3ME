#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Host tests for the XMB display gate and the GCM flip guard.

Both run the real sources (ps3/src/XmbGate.cpp, ps3/src/Ps3FlipGuard.cpp)
against host stand-ins for the PS3 headers, with the same link-time --wrap set
the PS3 application build uses.
"""
from pathlib import Path
import subprocess, tempfile

ROOT = Path(__file__).resolve().parents[1]
WARN = ['-std=c++11', '-Wall', '-Wextra', '-Werror', '-O1', '-g', '-fsanitize=address,undefined']


def test_xmb_gate():
    with tempfile.TemporaryDirectory(prefix='ps3me-xmb-gate-') as d:
        exe = Path(d) / 'xmb_gate_test'
        subprocess.run(['g++', *WARN, '-DPS3ME_HOST', '-I' + str(ROOT / 'ps3/include'),
                        str(ROOT / 'ps3/src/XmbGate.cpp'),
                        str(ROOT / 'tests/xmb_gate_test.cpp'), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


def test_flip_guard():
    with tempfile.TemporaryDirectory(prefix='ps3me-flip-guard-') as d:
        exe = Path(d) / 'flip_guard_test'
        subprocess.run(['g++', *WARN,
                        '-DPS3ME_HOST',
                        '-I' + str(ROOT / 'tests/stubs'),
                        '-I' + str(ROOT / 'ps3/include'),
                        str(ROOT / 'ps3/src/XmbGate.cpp'),
                        str(ROOT / 'ps3/src/Ps3FlipGuard.cpp'),
                        str(ROOT / 'tests/flip_guard_firmware.cpp'),
                        str(ROOT / 'tests/flip_guard_sdl.cpp'),
                        str(ROOT / 'tests/flip_guard_test.cpp'),
                        '-I' + str(ROOT / 'tests'),
                        '-Wl,--wrap=SDL_Flip',
                        '-Wl,--wrap=SDL_UpdateRect',
                        '-Wl,--wrap=SDL_UpdateRects',
                        '-Wl,--wrap=gcmGetFlipStatus',
                        '-Wl,--wrap=gcmResetFlipStatus',
                        '-Wl,--wrap=gcmSetFlip',
                        '-Wl,--wrap=gcmSetWaitFlip',
                        '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
