#!/usr/bin/env python3
"""First pointer-width fixes; intentionally does not claim a complete VM port."""
from pathlib import Path
import argparse
import shutil

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path, before, after):
    text = path.read_text()
    if after in text:
        return
    if before in text:
        if text.count(before) != 1:
            raise RuntimeError("ambiguous patch in " + str(path))
        path.write_text(text.replace(before, after))
    elif after not in text:
        raise RuntimeError("unexpected phoneME source in " + str(path))


def apply(source):
    utilities = source / "cldc/src/vm/share/utilities"
    shutil.copy2(ROOT / "vm/port/PointerMath.hpp", utilities / "PointerMath.hpp")
    globals_h = utilities / "GlobalDefinitions.hpp"
    replace_once(globals_h,
                 "#define DISTANCE(low, high)        (((jint) (high)) - ((jint) (low)))\n"
                 "#define DERIVED(type, ptr, offset) ((type) (((jint) (ptr))+((jint) (offset))))",
                 '#include "PointerMath.hpp"')
    replace_once(globals_h, "  int      _offset;", "  intptr_t _offset;")
    replace_once(globals_h, "_heap_array(NULL), _offset((int) native_addr)",
                 "_heap_array(NULL), _offset((intptr_t) native_addr)")
    replace_once(source / "cldc/src/vm/includeDB",
                 "GlobalDefinitions.hpp            GlobalDefinitions_<compiler>.hpp",
                 "GlobalDefinitions.hpp            GlobalDefinitions_<compiler>.hpp\n"
                 "GlobalDefinitions.hpp            PointerMath.hpp")
    replace_once(source / "cldc/src/vm/share/handles/WeakRefArray.hpp",
                 "return unsigned(unsigned(obj)+1) > unsigned(1);",
                 "return obj != NULL && obj != unused();")
    replace_once(source / "cldc/src/vm/share/memory/LargeObject.hpp",
                 "static void must_be_aligned ( const unsigned x )",
                 "static void must_be_aligned ( const uintptr_t x )")
    replace_once(source / "cldc/src/vm/share/memory/LargeObject.hpp",
                 "must_be_aligned( unsigned(x) );",
                 "must_be_aligned( uintptr_t(x) );")
    replace_once(source / "cldc/src/vm/share/memory/ExecutionStackDesc.hpp",
                 "    juint t = (juint) this;\n"
                 "    juint p = (juint) ptr;\n"
                 "    return ((p - t) < (juint)object_size());",
                 "    uintptr_t t = (uintptr_t) this;\n"
                 "    uintptr_t p = (uintptr_t) ptr;\n"
                 "    return ((p - t) < (uintptr_t)object_size());")
    for kind in ("text", "data"):
        replace_once(source / "cldc/src/vm/share/ROM/ROM.hpp",
                     "juint offset = ((juint)target) - ((juint)&_rom_%s_block[0]);" % kind,
                     "uintptr_t offset = ((uintptr_t)target) - ((uintptr_t)&_rom_%s_block[0]);" % kind)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phoneme", type=Path)
    args = parser.parse_args()
    apply(args.phoneme.resolve())
    print("Native address arithmetic fixed; 64-bit object layout still pending")
