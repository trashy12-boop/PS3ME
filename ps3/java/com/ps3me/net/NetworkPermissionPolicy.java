/* SPDX-License-Identifier: GPL-2.0-only */
package com.ps3me.net;
import com.sun.midp.security.Permissions;
/** Console network access for deliberately launched local MIDlets.
 * Phone-only interactive network permission levels are resolved for the session.
 * Explicit denial, forbidden APIs and every non-network permission stay intact.
 */
public final class NetworkPermissionPolicy {
    private NetworkPermissionPolicy() {}
    public static boolean isNetwork(String permission) {
        return "javax.microedition.io.Connector.socket".equals(permission) ||
               "javax.microedition.io.Connector.serversocket".equals(permission) ||
               "javax.microedition.io.Connector.datagram".equals(permission) ||
               "javax.microedition.io.Connector.datagramreceiver".equals(permission) ||
               "javax.microedition.io.Connector.http".equals(permission) ||
               "javax.microedition.io.Connector.https".equals(permission) ||
               "javax.microedition.io.Connector.ssl".equals(permission);
    }
    public static boolean allowInteractive(String permission,int level,int session,int maximum) {
        if(!isNetwork(permission) ||
           !(maximum==Permissions.ALLOW || maximum==Permissions.BLANKET_GRANTED ||
             maximum==Permissions.BLANKET || maximum==Permissions.SESSION ||
             maximum==Permissions.ONESHOT) ||
           session==Permissions.SESSION_DENIED) return false;
        return level==Permissions.SESSION || level==Permissions.BLANKET ||
               level==Permissions.ONESHOT;
    }
}
