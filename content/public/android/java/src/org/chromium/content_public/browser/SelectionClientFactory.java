// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.content.browser.SmartSelectionClient;
import org.chromium.ui.base.WindowAndroid;

/**
 * A public API to create {@link SelectionClient} instances.
 */
public class SelectionClientFactory {
    /** We create {@link SmartSelectionClient} instances. */
    public static SelectionClient create(SelectionClient.ResultCallback callback,
            WindowAndroid windowAndroid, WebContents webContents) {
        return SmartSelectionClient.create(callback, windowAndroid, webContents);
    }
}
