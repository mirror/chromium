// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.content.browser.GestureListenerManagerImpl;
import org.chromium.content.browser.webcontents.WebContentsImpl;

/**
 * Manages the {@link GestureStateListener} instances observing various gesture
 * events notifications from content layer.
 */
public interface GestureListenerManager extends WebContentsUserData {
    /**
     * @param webContents {@link WebContents} object.
     * @return {@link GestureListenerManager} object used for the give WebContents.
     *         Creates one if not present.
     */
    static GestureListenerManager fromWebContents(WebContents webContents) {
        WebContentsImpl webContentsImpl = (WebContentsImpl) webContents;
        GestureListenerManagerImpl manager =
                webContentsImpl.getUserData(GestureListenerManagerImpl.class);
        if (manager == null) {
            manager = new GestureListenerManagerImpl(webContentsImpl);
            webContentsImpl.setUserData(GestureListenerManagerImpl.class, manager);
        }
        return manager;
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
