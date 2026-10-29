#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Install genuine PSL1GHT VM OS adapters for the dedicated PPU configuration."""
from pathlib import Path
import shutil
from patch_utils import replace
ROOT=Path(__file__).resolve().parents[1]
def apply(source):
    replace(source/'cldc/src/vm/os/generic/OsFile_generic.hpp',
            "const char OsFile_path_separator_char = ';';",
            "const char OsFile_path_separator_char = ':';")
    for p in (ROOT/'vm/ps3_os').glob('*.cpp'):
        shutil.copy2(p,source/'cldc/src/vm/os/generic'/p.name)
    p=source/'cldc/src/vm/os/generic/OsSocket_generic.cpp'
    replace(p,'  // IMPL_NOTE: consider whether it should be fixed. \n',
            '  // No network operations are exposed by this offline port.\n'
            '  // Yield while Java threads are sleeping instead of busy spinning.\n'
            '  (void)blocked_threads; (void)blocked_threads_count;\n'
            '  Os::sleep(timeout_ms<0 ? 10 : (timeout_ms>50 ? 50 : timeout_ms));\n')
    p=source/'cldc/src/vm/cpu/c/Interpreter_c.cpp'
    replace(p,'  static inline void check_timer_tick() {',
            '  static inline void check_timer_tick() {\n'
            '#if defined(PS3ME_PPU)\n    extern void ps3me_poll_ticks_bridge();\n'
            '    ps3me_poll_ticks_bridge();\n#endif')
    text=p.read_text()
    if 'void ps3me_poll_ticks_bridge() {' not in text:
        text+='\n#if defined(PS3ME_PPU)\nextern "C" void ps3me_vm_poll_ticks();\n'
        text+='extern "C" void ps3me_poll_ticks_bridge() { ps3me_vm_poll_ticks(); }\n#endif\n'
        p.write_text(text)
    text=p.read_text().replace('\nvoid ps3me_poll_ticks_bridge() {','\nextern "C" void ps3me_poll_ticks_bridge() {')
    if p.read_text()!=text:p.write_text(text)
