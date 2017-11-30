// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.webcontents;

import org.chromium.content.browser.GestureListenerManagerImpl;
import org.chromium.content_public.browser.GestureListenerManager;

import java.util.HashMap;

/**
 *
 */
/* package */ final class UserDataMap {
    private final HashMap<Class<? extends UserData>, UserData> mUserDataMap = new HashMap<>();
    private final WebContentsImpl mWebContents;

    public UserDataMap(WebContentsImpl webContents) {
        mWebContents = webContents;
    }

    /**
     * Populates the data map with the objects used by WebContents.
     */
    public void init() {
        mUserDataMap.put(
                GestureListenerManager.class, new GestureListenerManagerImpl(mWebContents));
    }

    public void setUserData(Class<? extends UserData> key, UserData userData) {
        assert !mUserDataMap.containsKey(key); // Do not allow duplicated UserData
        mUserDataMap.put(key, userData);
    }

    public UserData getUserData(Class<? extends UserData> key) {
        return mUserDataMap.get(key);
    }
}
