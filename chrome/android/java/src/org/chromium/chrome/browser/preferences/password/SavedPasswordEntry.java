// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import org.chromium.chrome.browser.widget.prefeditor.EditableOption;

/**
 * A class representing information about a saved password entry in Chrome's settngs.
 *
 * Note: This could be a nested class in the PasswordManagerHandler interface, but that would mean
 * that PasswordUIView, which implements that interface and references SavedPasswordEntry in some of
 * its JNI-registered methods, would need an explicit import of PasswordManagerHandler. That again
 * would violate our presubmit checks, and https://crbug.com/424792 indicates that the preferred
 * solution is to move the nested class to top-level.
 */
public final class SavedPasswordEntry extends EditableOption {
    private final String mUrl;
    private final String mName;
    private final String mPassword;

    public SavedPasswordEntry(String id, String url, String name, String password) {
        super(id, url, name, password, null);
        mUrl = url;
        mName = name;
        mPassword = password;
    }

    public String getUrl() {
        return mUrl;
    }

    public String getUserName() {
        return mName;
    }

    public String getPassword() {
        return mPassword;
    }
}
