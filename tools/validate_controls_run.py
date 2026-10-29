#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
from pathlib import Path
import argparse,json,re
p=argparse.ArgumentParser();p.add_argument('directory',type=Path);p.add_argument('--edited-menu',action='store_true');a=p.parse_args();s=(a.directory/'runtime.log').read_text();base=json.loads((a.directory/'result.json').read_text());pressed=re.findall(r'TOUCH TEST\] pressed (\d+),(\d+)',s);dragged=re.findall(r'TOUCH TEST\] dragged (\d+),(\d+)',s);released=re.findall(r'TOUCH TEST\] released (\d+),(\d+)',s)
r={'touch_capabilities_reported':'supported=true motion=true' in s,'press_coordinates':pressed,'drag_coordinates':dragged,'release_coordinates':released,'mapped_key_53_pressed':'key pressed 53' in s,'mapped_key_53_released':'key released 53' in s,'menu_edit_saved':'menu returned Cross=Key 5' in s,'returned_to_browser':base['native_menu_frames_observed']>=120 and -1 in base['midp_return_codes'],'emulator_fatal_error':base['emulator_fatal_error'],'executable_sha256':base['executable_sha256'],'physical_controller_tested':False,'console_cursor_tested':False}
r['passed']=bool(r['touch_capabilities_reported'] and pressed and dragged and released and int(released[-1][0])>int(pressed[0][0]) and r['mapped_key_53_pressed'] and r['mapped_key_53_released'] and r['returned_to_browser'] and not r['emulator_fatal_error'] and (not a.edited_menu or r['menu_edit_saved']))
(a.directory/'controls_result.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2));assert r['passed']
