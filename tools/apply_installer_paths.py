#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Preserve absolute POSIX file URLs; only strip the slash before a DOS drive."""
from patch_utils import replace

def apply(source):
    p=source/'midp/src/ams/installer/reference/classes/com/sun/midp/installer/Installer.java'
    text=p.read_text()
    old="if (path.charAt(0) == '/')"
    new="if (path.length() > 2 && path.charAt(0) == '/' && path.charAt(2) == ':')"
    if old in text:
        p.write_text(text.replace(old,new))
    elif new not in text:
        raise RuntimeError('Unrecognized Installer.getUrlPath implementation')
