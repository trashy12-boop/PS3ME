#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Run RPCS3 and its optional X server in one isolated execution session."""
from pathlib import Path
import argparse,os,subprocess,time
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('rpcs3','executable','config_root','cache_root','output'):
        p.add_argument('--'+name.replace('_','-'),type=Path,required=True)
    p.add_argument('--xvfb',type=Path);p.add_argument('--x-libs',type=Path)
    p.add_argument('--display',type=int,default=95)
    p.add_argument('--seconds',type=int,default=90);a=p.parse_args()
    if not 5<=a.seconds<=600:p.error('seconds must be between 5 and 600')
    a.output.mkdir(parents=True,exist_ok=True)
    env=os.environ.copy();env.update(XDG_CONFIG_HOME=str(a.config_root.resolve()),XDG_CACHE_HOME=str(a.cache_root.resolve()))
    server=None;server_log=None
    try:
        if a.xvfb:
            env.update(DISPLAY='127.0.0.1:'+str(a.display),QT_QPA_PLATFORM='xcb')
            if a.x_libs:env['LD_LIBRARY_PATH']=str(a.x_libs.resolve())
            server_log=(a.output/'xvfb.log').open('w')
            server=subprocess.Popen([str(a.xvfb.resolve()),':'+str(a.display),'-screen','0','1280x720x24','-ac','-nolisten','unix','-nolisten','local','-listen','tcp'],env=env,stdout=server_log,stderr=subprocess.STDOUT)
            time.sleep(2)
            if server.poll() is not None:raise RuntimeError('X server failed; see xvfb.log')
        with (a.output/'stdout.log').open('w') as log:
            process=subprocess.Popen([str(a.rpcs3.resolve()),'--headless',str(a.executable.resolve())],env=env,stdout=log,stderr=subprocess.STDOUT)
            try:code=process.wait(timeout=a.seconds);print('Emulator exit',code)
            except subprocess.TimeoutExpired:
                process.terminate()
                try:process.wait(timeout=10)
                except subprocess.TimeoutExpired:process.kill();process.wait()
                print('Stopped at requested test duration')
    finally:
        if server is not None:
            server.terminate();server.wait(timeout=5)
        if server_log:server_log.close()
if __name__=='__main__':main()
