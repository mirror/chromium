// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

/**
 * This interface may be optionally implemented by FRE fragments to be notified about native
 * initialization or intercept Back button.
 */
public interface FirstRunFragment {
    /**
     * Notifies this fragment that native has been initialized.
     */
    default void onNativeInitialized() {}

    /**
     * @return Whether the back button press was handled by this page.
     */
    default boolean interceptBackPressed() {
        return false;
    }
}
