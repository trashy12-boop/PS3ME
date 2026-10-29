#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run an installed PS3ME package in isolated RPCS3 directories and retain evidence.

Firmware and PKG must already be installed by RPCS3. Headless mode tests CPU
execution with the Null renderer; it cannot verify visible graphics or a pad.
"""
from pathlib import Path
import argparse, datetime, hashlib, json, os, re, shutil, subprocess, time

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rpcs3',type=Path,required=True)
    p.add_argument('--config-root',type=Path,required=True)
    p.add_argument('--cache-root',type=Path,required=True)
    p.add_argument('--emulator-config',type=Path)
    p.add_argument('--executable',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--mode',choices=('menu','diagnostic'),default='menu')
    p.add_argument('--seconds',type=int,default=45)
    p.add_argument('--return-after-ms',type=int,default=0)
    a=p.parse_args(); exe=a.executable.resolve(); out=a.output.resolve()
    assert exe.is_file() and exe.parent.name=='USRDIR', 'select an installed PS3ME executable'
    assert 5<=a.seconds<=600
    out.mkdir(parents=True,exist_ok=True)
    logs=exe.parent/'logs'; logs.mkdir(exist_ok=True)
    runtime=logs/'runtime.log'; offset=runtime.stat().st_size if runtime.exists() else 0
    trigger=logs/'diagnostic.once'
    if a.mode=='diagnostic':trigger.write_text(str(a.return_after_ms))
    else:trigger.unlink(missing_ok=True)
    env=os.environ.copy(); env.update(XDG_CONFIG_HOME=str(a.config_root.resolve()),XDG_CACHE_HOME=str(a.cache_root.resolve()))
    start=time.monotonic(); timed_out=False
    with (out/'stdout.log').open('wb') as stream:
        command=[str(a.rpcs3.resolve()),'--headless']
        if a.emulator_config:command+=['--config',str(a.emulator_config.resolve())]
        process=subprocess.Popen(command+[str(exe)],env=env,stdout=stream,stderr=subprocess.STDOUT)
        try:code=process.wait(timeout=a.seconds)
        except subprocess.TimeoutExpired:
            timed_out=True;process.terminate()
            try:code=process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();code=process.wait()
    text=runtime.read_bytes()[offset:].decode('utf-8','replace') if runtime.exists() else ''
    (out/'runtime.log').write_text(text)
    for name in ('RPCS3.log','TTY.log'):
        source=a.cache_root/'rpcs3'/name
        if source.is_file():shutil.copy2(source,out/name)
    emulator=(out/'RPCS3.log').read_text(errors='replace') if (out/'RPCS3.log').exists() else ''
    menu=max([int(x) for x in re.findall(r'menu frames: (\d+)',text)]+[0])
    errors=re.findall(r'^Uncaught exception:.*',text,re.M)
    fatal=bool(re.search(r'Access violation|Segfault|Booting .* failed|Narrowing error|Thread terminated due to fatal error|Fatal error:',emulator+text+(out/'stdout.log').read_text(errors='replace')))
    result={'date':datetime.datetime.now(datetime.timezone.utc).isoformat(),'mode':a.mode,
            'executable':exe.name,'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),
            'elapsed_seconds':round(time.monotonic()-start,2),'stopped_by_test_timeout':timed_out,'emulator_exit_code':code,
            'native_menu_frames_observed':menu,'native_menu_test_passed':menu>=120 and not fatal,
            'java_loader_requested':'starting GameLoader' in text,'uncaught_java_exceptions':errors,
            'java_launch_failures':re.findall(r'^.*\[PS3ME\] launch failed:.*',text,re.M),
            'diagnostic_started':'[PS3ME diagnostic] startApp' in text,
            'wide_constants_and_sleep_passed':'wide constants and sleep passed' in text,
            'allocation_gc_passed':'allocation and GC passed' in text,
            'rms_launch_counts':[int(x) for x in re.findall(r'RMS saved launches=(\d+)',text)],
            'canvas_paint_completed':'Canvas paint completed' in text,
            'rgb565_backend_received':'RGB565 diagnostic bands received' in text,
            'automated_start_requested':'requesting return through Start handler' in text,
            'midp_return_codes':[int(x) for x in re.findall(r'MIDP returned (-?\d+)',text)],
            'emulator_fatal_error':fatal,'renderer':'Null (headless)',
            'ppu_decoder':(re.findall(r'PPU Decoder: (.*)',emulator) or ['unknown'])[-1],
            'configuration_errors':re.findall(r'^.*invalid enum or integer.*',emulator,re.M),
            'visible_graphics_tested':False,'controller_tested':False,'console_runtime_tested':False}
    result['diagnostic_test_passed']=all((result['diagnostic_started'],
        result['wide_constants_and_sleep_passed'],result['allocation_gc_passed'],
        bool(result['rms_launch_counts']),result['canvas_paint_completed'],
        result['rgb565_backend_received'],not fatal,not result['java_launch_failures']))
    result['return_to_menu_test_passed']=bool(a.return_after_ms and result['diagnostic_test_passed']
        and result['automated_start_requested'] and result['midp_return_codes'] and menu>=120)
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
