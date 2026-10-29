#!/usr/bin/env python3
"""Apply narrowly scoped CLDC build fixes; this is NOT a 64-bit VM port.

The VM can be run through QEMU user mode on hosts without an i386 kernel ABI.
The runner only prefixes recipes that execute a generated VM, never targets.
"""
import argparse
from pathlib import Path


def prepare(root: Path) -> None:
    makefile = root / "cldc/build/share/jvm.make"
    text = makefile.read_text()
    if '.DELETE_ON_ERROR:' not in text:
        text='.DELETE_ON_ERROR:\n'+text
    text = text.replace("CPP_FLAGS               += -Werror",
                        "# PS3ME: legacy VM warnings do not fail modern compilers")
    text = text.replace("CPP_DEF_FLAGS           += -fstrict-aliasing",
                        "CPP_DEF_FLAGS           += -fno-strict-aliasing -fwrapv")
    text = text.replace("CPP_DEF_FLAGS           += -pipe -DGCC",
                        "CPP_DEF_FLAGS           += -D_FILE_OFFSET_BITS=64 -Wno-narrowing -std=gnu++03 -fno-pie -pipe -DGCC")
    text=text.replace(' -pipe -DGCC',' -DGCC')
    # Generated pointers in the original VM assume a fixed non-PIE image.
    text = text.replace("LINK_FLAGS             += -m32\n",
                        "LINK_FLAGS             += -m32 -no-pie\n")
    lines = text.splitlines(keepends=True)
    for i, line in enumerate(lines):
        if line.startswith("\t") and "$(HOST_VM_RUNNER)" not in line:
            if "echo " not in line and "$(MAKE)" not in line:
                for name in ("ROM_GENERATOR", "LOOP_GENERATOR", "PREVERIFY"):
                    # Match an executable, not a reference in test/chmod commands.
                    line = line.replace("\t$(A)$(" + name + ") ", "\t$(A)$(HOST_VM_RUNNER) $(" + name + ") ")
                    line = line.replace("\t$(" + name + ") ", "\t$(HOST_VM_RUNNER) $(" + name + ") ")
            lines[i] = line
    text="".join(lines)
    if 'HOST_PREVERIFY_RUNNER ?=' not in text:
        text='HOST_PREVERIFY_RUNNER ?= $(HOST_VM_RUNNER)\n'+text
    text=text.replace('$(HOST_VM_RUNNER) $(PREVERIFY)', '$(HOST_PREVERIFY_RUNNER) $(PREVERIFY)')
    makefile.write_text(text)
    asm = root / "cldc/src/vm/cpu/c/AsmStubs_i386.s"
    text = asm.read_text()
    if ".arch .387" not in text:
        text = text.replace(".arch i486", ".arch i486\n.arch .387")
    asm.write_text(text)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phoneme", type=Path)
    args = parser.parse_args()
    prepare(args.phoneme)
    print("CLDC baseline build compatibility patches applied")
