// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.webcontents;

import org.chromium.content_public.browser.SupportsUserData.Data;

import java.util.HashMap;

/**
 *
 */
/* package */ final class UserDataMap {
    private final HashMap<Class<? extends Data>, Data> mUserDataMap = new HashMap<>();
    private final WebContentsImpl mWebContents;

    public UserDataMap(WebContentsImpl webContents) {
        mWebContents = webContents;
    }

    public void setUserData(Class<? extends Data> key, Data data) {
        assert !mUserDataMap.containsKey(key); // Do not allow duplicated Data
        mUserDataMap.put(key, data);
    }

    public Data getUserData(Class<? extends Data> key) {
        return mUserDataMap.get(key);
    }
}
