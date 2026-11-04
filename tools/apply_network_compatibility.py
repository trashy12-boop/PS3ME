#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""HTTP extensions required by modern J2ME applications (including Discord)."""
from pathlib import Path
from patch_utils import replace

def apply(source):
    p=source/'midp/src/protocol/http/reference/classes/com/sun/midp/io/j2me/http/Protocol.java'
    replace(p, '!method.equals(POST)) {',
        '!method.equals(POST) &&\n'
        '            !method.equals("PUT") &&\n'
        '            !method.equals("DELETE") &&\n'
        '            !method.equals("PATCH") &&\n'
        '            !method.equals("OPTIONS")) {')

if __name__=='__main__':
    apply(Path(__file__).resolve().parents[1]/'deps/phoneME')
