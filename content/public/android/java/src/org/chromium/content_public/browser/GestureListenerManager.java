// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.content.browser.webcontents.UserData;
import org.chromium.content.browser.webcontents.WebContentsImpl;

/**
 *
 */
public interface GestureListenerManager extends UserData {
    /**
     *
     */
    static GestureListenerManager fromWebContents(WebContents webContents) {
        WebContentsImpl webContentsImpl = (WebContentsImpl) webContents;
        return (GestureListenerManager) webContentsImpl.getUserData(GestureListenerManager.class);
    }

    /**
     * Add a listener that gets alerted on gesture state changes.
     * @param listener Listener to add.
     */
    void addListener(GestureStateListener listener);

    /**
     * Removes a listener that was added to watch for gesture state changes.
     * @param listener Listener to remove.
     */
    void removeListener(GestureStateListener listener);
}
