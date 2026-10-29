#!/usr/bin/env python3
"""Build and execute the original 32-bit C interpreter as a regression baseline.

Needs JDK 8, GNU Make and an i386-capable GCC/G++ toolchain (e.g. multilib).
--runner accepts an executable wrapper for hosts requiring qemu-i386.
This validates the CLDC engine on the host; it does not build a PS3 emulator.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import time

from prepare_vm_baseline import prepare

ROOT = Path(__file__).resolve().parents[1]


def run(command, log, env, timeout=600):
    started = time.monotonic()
    with log.open("w") as stream:
        stream.write("$ " + shlex.join(map(str, command)) + "\n")
        stream.flush()
        result = subprocess.run(list(map(str, command)), stdout=stream,
                                stderr=subprocess.STDOUT, env=env, timeout=timeout)
    if result.returncode:
        raise RuntimeError("command failed (%d); see %s" % (result.returncode, log))
    return round(time.monotonic() - started, 3)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jdk", type=Path, required=True)
    parser.add_argument("--compiler-dir", type=Path)
    parser.add_argument("--runner", type=Path)
    parser.add_argument("--phoneme", type=Path, default=ROOT / "deps/phoneME")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    source = args.phoneme.resolve()
    jdk = args.jdk.resolve()
    if not (jdk / "bin/javac").is_file():
        parser.error("--jdk must contain bin/javac (JDK 8)")
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    env = os.environ.copy()
    if args.compiler_dir:
        env["PATH"] = str(args.compiler_dir.resolve()) + os.pathsep + env["PATH"]
    runner = [str(args.runner.resolve())] if args.runner else []
    output = ROOT / "build/vm32"
    validation = ROOT / "build/validation"
    validation.mkdir(parents=True, exist_ok=True)
    report_path = validation / "vm_baseline_result.json"
    # An earlier successful result must not survive a failed rebuild.
    report = {"status": "running", "platform": "host-i386", "ps3_runtime_tested": False}
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    try:
        prepare(source)
        pvbuild = source / "preverifier/build/linux"
        # Rebuild the verifier: its old prebuilt readdir() can silently skip
        # files on modern filesystems with inode numbers exceeding 32 bits.
        run(["make", "-C", pvbuild, "clean"], validation / "preverify_clean.log", env)
        run(["make", "-C", pvbuild, "-j%d" % args.jobs,
             "EXTRACFLAGS=-m32 -D_FILE_OFFSET_BITS=64 -std=gnu89 -fcommon",
             "LDFLAGS=-m32 -no-pie"], validation / "preverify_build.log", env)
        preverify = pvbuild / "preverify"
        installed = output / "linux_c/dist/bin/preverify"
        installed.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(preverify, installed)
        classes = output / "classes.zip"
        # Re-preverify classes using the rebuilt tool, including on incremental runs.
        classes.unlink(missing_ok=True)
        for name in ("classes", "tmpclasses"):
            generated = output / name
            if generated.exists():
                shutil.rmtree(generated)
        build_seconds = run([
            "make", "-C", source / "cldc/build/linux_c", "product",
            "host_arch=i386", "JDK_DIR=" + str(jdk),
            "JVMWorkSpace=" + str(source / "cldc"), "JVMBuildSpace=" + str(output),
            "HOST_VM_RUNNER=" + shlex.join(runner),
            "PREVERIFY_ORIGINAL=" + str(preverify),
            # Explicit link libraries retain the math routines with modern GCC.
            "LINK_FLAGS=-m32 -no-pie -lm -lpthread -lrt",
            "PARALLEL_ARGS=-j%d" % args.jobs], validation / "vm32_build.log", env)
        if not classes.is_file() or classes.stat().st_size < 1024:
            raise RuntimeError("CLDC class library was not produced")
        fixtures = ROOT / "build/vm_tests"
        if fixtures.exists():
            shutil.rmtree(fixtures)
        raw = fixtures / "raw"
        verified = fixtures / "verified"
        raw.mkdir(parents=True, exist_ok=True)
        verified.mkdir(parents=True, exist_ok=True)
        run([jdk / "bin/javac", "-source", "1.4", "-target", "1.4",
             "-bootclasspath", classes, "-d", raw,
             ROOT / "tests/vm_baseline/VmBaseline.java"],
            validation / "vm_java_compile.log", env)
        run(runner + [str(preverify), "-classpath", str(classes) + ":" + str(raw),
                      "-d", str(verified), str(raw)], validation / "vm_java_preverify.log", env)
        if not (verified / "VmBaseline.class").is_file():
            raise RuntimeError("preverify did not produce VmBaseline.class")
        vm = output / "linux_c/dist/bin/cldc_vm"
        runtime = run(runner + [str(vm), "=HeapCapacity2M", "-cp", str(verified), "VmBaseline"],
                      validation / "vm_java_run.log", env, timeout=60)
        success = "PS3ME_VM_BASELINE_OK checks=18"
        if success not in (validation / "vm_java_run.log").read_text():
            raise RuntimeError("VM exited without the Java success marker")
        report.update(status="passed", checks=18, marker=success,
                      build_seconds=build_seconds, execution_seconds=runtime,
                      java_vm="phoneME Feature CLDC 1.1 C interpreter, i386",
                      clean_class_and_test_outputs=True,
                      pointer_fixes_applied='#include "PointerMath.hpp"' in
                          (source / "cldc/src/vm/share/utilities/GlobalDefinitions.hpp").read_text(),
                      covers=["int/long/float/double arithmetic", "object/array/static GC roots",
                              "interface dispatch", "native arraycopy and strings", "exceptions/finally",
                              "green threads/monitors", "sleep/clock"],
                      can_run_midlets=False, ps3_vm_port_complete=False)
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        report.update(status="failed", error=str(error))
        report_path.write_text(json.dumps(report, indent=2) + "\n")
        print(str(error), file=sys.stderr)
        return 1
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    print(success)
    print("Host baseline passed; the native PS3 VM is still incomplete.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
