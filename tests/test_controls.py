#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
from pathlib import Path
import os,subprocess,tempfile
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='ps3me-controls-') as d:
 exe=Path(d)/'controls_test'
 subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-g','-Wl,--wrap=rename','-I'+str(r/'ps3/include'),'-I'+str(r/'upstream/ps2/javacall'),str(r/'ps3/src/Controls.cpp'),str(r/'ps3/src/Video.cpp'),str(r/'tests/controls_test.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe),str(Path(d)/'controls.cfg')],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),check=True)
