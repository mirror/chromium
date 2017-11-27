// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.languages;

/**
 * Simple object representing the languge item.
 */
public class LanguageItem {
    // This ISO code of the language.
    private final String mCode;

    // The display name of the language in the current locale.
    private final String mDisplayName;

    // The display name of the language in the language locale.
    private final String mNativeDisplayName;

    // Whether we support translate for this language.
    private final boolean mSupportTranslate;

    // Whether this language is a accept language.
    private boolean mAccepted = false;

    public LanguageItem(
            String code, String displayName, String nativeDisplayName, boolean supportTranslate) {
        mCode = code;
        mDisplayName = displayName;
        mNativeDisplayName = nativeDisplayName;
        mSupportTranslate = supportTranslate;
    }

    public void markAccepted(boolean accepted) {
        mAccepted = accepted;
    }

    public String getCode() {
        return mCode;
    }

    public String getDisplayName() {
        return mDisplayName;
    }

    public String getNativeDisplayName() {
        return mNativeDisplayName;
    }

    public boolean isSupported() {
        return mSupportTranslate;
    }
}
