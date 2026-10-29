#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name('build_timidity.py')),run_name='__main__')
