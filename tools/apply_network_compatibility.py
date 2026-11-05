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


    security=source/'midp/src/security/permission_dialog/reference/classes/com/sun/midp/security/SecurityHandler.java'
    replace(security, '                switch (permissions[permission]) {',
        '                /* PS3ME: resolve interactive network levels for this session. */\n'
        '                if (com.ps3me.net.NetworkPermissionPolicy.allowInteractive(\n'
        '                        permissionStr, permissions[permission], sessionValues[permission],\n'
        '                        maxPermissionLevels[permission])) {\n'
        '                    sessionValues[permission] = Permissions.SESSION_GRANTED;\n'
        '                    System.out.println("[PS3ME net] network permission granted for session: " + permissionStr);\n'
        '                    return false;\n'
        '                }\n'
        '                switch (permissions[permission]) {')
    socket=source/'midp/src/protocol/socket/reference/classes/com/sun/midp/io/j2me/socket/Protocol.java'
    replace(socket, '            AccessController.checkPermission(CLIENT_PERMISSION_NAME, name);',
        '            System.out.println("[PS3ME net] checking socket permission");\n'
        '            AccessController.checkPermission(CLIENT_PERMISSION_NAME, name);\n'
        '            System.out.println("[PS3ME net] socket permission returned");')

if __name__=='__main__':
    apply(Path(__file__).resolve().parents[1]/'deps/phoneME')
