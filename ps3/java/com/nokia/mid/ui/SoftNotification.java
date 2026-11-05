/* SPDX-License-Identifier: GPL-2.0-only
 * Type linkage for optional Nokia notification paths. The capability property
 * stays absent, so applications can disable this unsupported phone feature.
 */
package com.nokia.mid.ui;
public abstract class SoftNotification {
    protected SoftNotification() {}
    public static SoftNotification newInstance() {
        throw new RuntimeException("Nokia notifications are unavailable on PS3");
    }
    public abstract void setText(String groupText, String text);
    public abstract void post();
    public abstract void remove();
}
