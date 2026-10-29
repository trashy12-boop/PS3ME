#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Generate JavaCall's real static property database from MIDP's merged XML."""
from pathlib import Path
import sys,json,xml.etree.ElementTree as ET
source,target=map(Path,sys.argv[1:])
groups={'application':{},'internal':{}}
for node in ET.parse(source).iter('property'):
    key=node.get('Key');value=node.get('Value');scope=node.get('Scope','application')
    if scope=='system':scope='application'
    if key and value is not None and scope in groups:groups[scope][key]=value
def c(text):return json.dumps(text,ensure_ascii=True)
lines=['/* Generated from MIDP configuration. */','#include <stddef.h>']
for scope,values in groups.items():
    for kind,items in [('keys',values.keys()),('values',values.values())]:
        lines.append('static char* %s_%s[]={%s};'%(scope,kind,','.join([c(x) for x in items]+['NULL'])))
lines+=['char* javacall_static_properties_sections[]={"application","internal",NULL};',
        'char** javacall_static_properties_keys[]={application_keys,internal_keys,NULL};',
        'char** javacall_static_properties_values[]={application_values,internal_values,NULL};']
target.write_text('\n'.join(lines)+'\n')
