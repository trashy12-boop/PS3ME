#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Check native PPU metadata and PSL1GHT post-link fixups before packaging."""
from pathlib import Path
import argparse, hashlib, json, struct

def validate(path):
    data = Path(path).read_bytes()
    assert data[:6] == b'\x7fELF\x02\x02', 'expected big-endian ELF64'
    assert data[7] == 102, 'missing Cell LV2 OS ABI (run sprxlinker)'
    assert struct.unpack_from('>HH', data, 16) == (2, 21), 'expected PPC64 executable'
    phoff, shoff = struct.unpack_from('>QQ', data, 32)
    phsize, phnum, shsize, shnum, shstr = struct.unpack_from('>HHHHH', data, 54)
    ph = [struct.unpack_from('>IIQQQQQQ', data, phoff + i * phsize) for i in range(phnum)]
    params = [p for p in ph if p[0] == 0x60000001]
    assert len(params) == 1 and params[0][5] >= 32, 'missing SYS_PROCESS_PARAM'
    proc = struct.unpack_from('>8I', data, params[0][2])
    assert proc[0] == 32 and proc[1] == 0x13bcc5f6, 'invalid process parameters'
    assert proc[4] > 0 and proc[5] >= 0x10000, 'invalid priority/stack size'
    sections = [struct.unpack_from('>IIQQQQIIQQ', data, shoff + i * shsize) for i in range(shnum)]
    strings = sections[shstr]; names = data[strings[4]:strings[4] + strings[5]]
    sections = {names[s[0]:].split(b'\0', 1)[0].decode(): s for s in sections}
    opd = sections['.opd']; assert opd[5] > 0 and opd[5] % 24 == 0
    for offset in range(opd[4], opd[4] + opd[5], 24):
        function, toc, descriptor = struct.unpack_from('>QQQ', data, offset)
        assert function <= 0xffffffff and toc <= 0xffffffff, 'PPU address out of range'
        assert descriptor == (function << 32 | toc), 'unfixed function descriptor (run sprxlinker)'
        assert any(p[0] == 1 and p[1] & 1 and p[3] <= function < p[3] + p[6] for p in ph), 'function outside executable segment'
    stubs = sections['.lib.stub']; assert stubs[5] > 0 and stubs[5] % 44 == 0
    imports = [struct.unpack_from('>H', data, offset + 6)[0] for offset in range(stubs[4], stubs[4] + stubs[5], 44)]
    assert all(imports), 'unfixed import counts (run sprxlinker)'
    return {'status': 'passed', 'architecture': 'ELF64 big-endian PowerPC64',
            'os_abi': 'Cell LV2', 'process_priority': proc[4], 'stack_bytes': proc[5],
            'fixed_function_descriptors': opd[5] // 24, 'library_import_counts': imports,
            'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
            'console_runtime_tested': False}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path); parser.add_argument('--output', type=Path)
    args = parser.parse_args(); result = json.dumps(validate(args.elf), indent=2) + '\n'
    if args.output: args.output.write_text(result)
    print(result, end='')
