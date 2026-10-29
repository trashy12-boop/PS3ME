#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Checked, idempotent substitutions for the pinned upstream source."""
def write_text_if_changed(path,text):
    if path.read_text()!=text:path.write_text(text)

def replace(path,before,after,all_occurrences=False):
    text=path.read_text(encoding='latin1')
    token='\x00PS3ME_APPLIED\x00'
    pending=text.replace(after,token)
    count=pending.count(before)
    if not count:
        if after in text:return
        raise RuntimeError('Unexpected source: '+str(path)+': '+repr(before))
    if count!=1 and not all_occurrences:
        raise RuntimeError('Ambiguous source: '+str(path)+': '+repr(before))
    result=pending.replace(before,after).replace(token,after)
    if result!=text:path.write_bytes(result.encode('latin1'))
