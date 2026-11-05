/* SPDX-License-Identifier: GPL-2.0-only
 * No file provider is installed: accurately expose no accessible JSR-75 roots.
 */
package javax.microedition.io.file;
import java.util.Enumeration;
import java.util.Vector;
public final class FileSystemRegistry {
    private FileSystemRegistry() {}
    public static Enumeration listRoots() { return new Vector().elements(); }
}
