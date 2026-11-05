/* SPDX-License-Identifier: GPL-2.0-only */
import com.ps3me.net.NetworkPermissionPolicy;
import com.sun.midp.security.Permissions;
public final class NetworkPermissionPolicyTest {
    private static void require(boolean value) { if(!value) throw new RuntimeException("policy regression"); }
    public static void main(String[] args) {
        String[] allowed={"socket","serversocket","datagram","datagramreceiver","http","https","ssl"};
        int[] prompt={Permissions.SESSION,Permissions.BLANKET,Permissions.ONESHOT};
        int[] deny={Permissions.NEVER,Permissions.BLANKET_DENIED,Permissions.SESSION_DENIED,-7};
        for(int i=0;i<allowed.length;i++) {
            String p="javax.microedition.io.Connector."+allowed[i];
            for(int j=0;j<prompt.length;j++) {
                require(NetworkPermissionPolicy.allowInteractive(p,prompt[j],Permissions.SESSION_UNKNOWN,Permissions.SESSION));
                require(!NetworkPermissionPolicy.allowInteractive(p,prompt[j],Permissions.SESSION_DENIED,Permissions.SESSION));
                require(!NetworkPermissionPolicy.allowInteractive(p,prompt[j],Permissions.SESSION_UNKNOWN,Permissions.NEVER));
            }
            for(int j=0;j<deny.length;j++) require(!NetworkPermissionPolicy.allowInteractive(p,deny[j],Permissions.SESSION_UNKNOWN,Permissions.SESSION));
        }
        String[] others={null,"com.sun.midp","com.sun.midp.ams","com.sun.midp.trustedapp","javax.microedition.io.Connector.file.read","javax.microedition.io.Connector.file.write","javax.microedition.io.Connector.socket.fake","javax.wireless.messaging.sms.send"};
        for(int i=0;i<others.length;i++) require(!NetworkPermissionPolicy.allowInteractive(others[i],Permissions.SESSION,Permissions.SESSION_UNKNOWN,Permissions.SESSION));
        System.out.println("PASS seven network permissions; explicit denials, forbidden maximums and non-network APIs preserved");
    }
}
