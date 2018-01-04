// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.app.Fragment;

/**
 * Represents first run page shown during the First Run. Actual page implementation is created
 * lazily by {@link #instantiateFragment()}.
 */
public interface FirstRunPage {
    /**
     * @return Whether this page should be skipped on the FRE creation.
     */
    default boolean shouldSkipPageOnCreate() {
        return false;
    }

    /**
     * Creates fragment that implements this FRE page. Fragment may optionally implement
     * {@link FirstRunFragment} to be notified about native initialization or intercept Back button.
     */
    Fragment instantiateFragment();
}
