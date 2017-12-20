// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Since the ADM can only be accessed from Java, this bridge will transfer all C++ calls over to
 * Java land for making the call to ADM.  This is a one-way bridge, from C++ to Java only. The
 * Java side of this bridge is not called by other Java code.
 */
@JNINamespace("offline_pages::android")
public class OfflinePagesDownloadManagerBridge {
    /**
     * Checks to see if ADM is installed on this phone.
     * Returns true if the android download app is installed.
     */
    @CalledByNative
    private static boolean isAndroidDownloadManagerInstalled() {
        // TODO(petewil): Implement
        return true;
    }

    /** This is a pass through to the ADM function of the same name. */
    @CalledByNative
    private static long addCompletedDownload(String title, String description,
            boolean isMediaScannerScannable, String mimeType, String path, long length,
            boolean showNotification, String uri, String referer) {
        // TODO(petewil): implement
        return 0;
    }

    /** This is a pass through to the ADM function of the same name. */
    @CalledByNative
    private static int remove(long[] ids) {
        // TODO(petewil): implement
        return 0;
    }
}
