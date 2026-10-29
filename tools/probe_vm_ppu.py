#!/usr/bin/env python3
"""Attempt an actual PPU CLDC compile and save the remaining compiler failures.

This is a development probe, not a playable build. The upstream generic OS
placeholders are used while addressing the common VM memory model. ROM and
MIDP integration are deliberately not claimed or exported by this probe.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

from prepare_vm_baseline import prepare
from apply_vm_native64 import apply

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jdk", type=Path, required=True)
    parser.add_argument("--runner", type=Path)
    parser.add_argument("--sdk", type=Path, default=ROOT / "deps/ps3dev")
    parser.add_argument("--phoneme", type=Path, default=ROOT / "deps/phoneME")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--keep-going",action="store_true")
    parser.add_argument("--oopmaps",type=Path)
    args = parser.parse_args()
    source = args.phoneme.resolve()
    sdk = args.sdk.resolve()
    jdk = args.jdk.resolve()
    preverify = source / "preverifier/build/linux/preverify"
    if not preverify.is_file():
        parser.error("run build_vm_baseline.py first to build the host preverify tool")
    if not (sdk / "ppu/bin/ppu-g++").is_file():
        parser.error("--sdk does not contain the PPU compiler")
    prepare(source)
    apply(source)
    from apply_ps3_os import apply as os_port
    os_port(source)
    config = source / "cldc/build/ps3_ppu"
    config.mkdir(exist_ok=True)
    for name in ("Makefile", "ps3_ppu.cfg"):
        shutil.copy2(ROOT / "vm/ps3_ppu" / name, config / name)
    shutil.copy2(ROOT / "vm/ps3_ppu/AsmStubs_ppu.s",
                 source / "cldc/src/vm/cpu/c/AsmStubs_ppu.s")
    output = ROOT / "build/vm_ppu"
    installed = output / "ps3_ppu/dist/bin/preverify"
    installed.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(preverify, installed)
    # A failed historical preverification must not leave a fake empty ZIP.
    (output / "classes.zip").unlink(missing_ok=True)
    log = ROOT / "build/validation/vm_ppu_build.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    runner = [str(args.runner.resolve())] if args.runner else []
    command = ["make", "-C", str(config), "tools", "_product", "ROMIZING=false",
               "JDK_DIR=" + str(jdk), "JVMWorkSpace=" + str(source / "cldc"),
               "JVMBuildSpace=" + str(output), "PS3DEV=" + str(sdk),
               "HOST_VM_RUNNER=" + shlex.join(runner),
               "PREVERIFY_ORIGINAL=" + str(preverify),
               "PARALLEL_ARGS=-j%d" % args.jobs]
    if args.oopmaps:
        command.append("PS3ME_NATIVE_OOPMAPS="+str(args.oopmaps.resolve()))
    result_path = log.with_name("vm_ppu_result.json")
    report = {"platform": "PS3 PPU / PSL1GHT / ELF64 big-endian",
              "status": "running", "vm_complete": False,
              "console_runtime_tested": False, "can_run_midlets": False,
              "os_integration": "PSL1GHT filesystem, time, memory and cooperative ticker; execution unverified",
              "diagnostic_scope": "compiler stopped at initial shared headers; this list is not an exhaustive VM port audit"}
    result_path.write_text(json.dumps(report, indent=2) + "\n")
    try:
        with log.open("w") as stream:
            stream.write("$ " + shlex.join(command) + "\n")
            stream.flush()
            build = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT,
                                   env=os.environ.copy(), timeout=300)
        diagnostics = set()
        for line in log.read_text().splitlines():
            match = re.search(r"/cldc/(.*?:[0-9]+:[0-9]+: (?:fatal )?error: .*)", line)
            if match:
                diagnostics.add("cldc/" + match.group(1))
        report.update(status="compile_blocked" if build.returncode else "compile_only",
                      build_exit_code=build.returncode,
                      compiler_errors=sorted(diagnostics),
                      object_files=sum(f.read_bytes()[:4]==b"\x7fELF" for f in (output / "ps3_ppu/target/product").glob("*.o")))
    except (OSError, subprocess.TimeoutExpired) as error:
        report.update(status="probe_failed", error=str(error))
        result_path.write_text(json.dumps(report, indent=2) + "\n")
        print(str(error), file=sys.stderr)
        return 1
    result_path.write_text(json.dumps(report, indent=2) + "\n")
    print("PPU CLDC compile: " + report["status"])
    print("Unique compiler errors: " + str(len(report["compiler_errors"])))
    print("Report: " + str(result_path))
    return 2 if build.returncode else 0


if __name__ == "__main__":
    sys.exit(main())
