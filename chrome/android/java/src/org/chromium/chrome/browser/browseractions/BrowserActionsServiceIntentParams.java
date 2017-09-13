// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.browseractions;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

/**
 * A container object to hold parameters for constructing Intent for {@link BrowserActionsService}.
 */
public class BrowserActionsServiceIntentParams {
    private final String mAction;
    private final String mLinkUrl;
    private final String mSourcePackageName;

    private BrowserActionsServiceIntentParams(
            String action, @Nullable String linkUrl, @Nullable String sourcePackageName) {
        mAction = action;
        mLinkUrl = linkUrl;
        mSourcePackageName = sourcePackageName;
    }

    /**
     * @return The action for the Intent.
     */
    public String getAction() {
        return mAction;
    }

    /**
     * @return The url for the tab created by the Intent.
     */
    public String getLinkUrl() {
        return mLinkUrl;
    }

    /**
     * @return The source package name which requests the tab creation.
     */
    public String getSourcePackageName() {
        return mSourcePackageName;
    }

    /** The builder for {@link BrowserActionsServiceIntentParams} objects. */
    public static class Builder {
        private String mAction;
        private String mLinkUrl;
        private String mSourcePackageName;

        public Builder(@NonNull String action) {
            mAction = action;
        }

        /**
         * Sets the url for the tab created by the Intent.
         */
        public Builder setLinkUrl(String linkUrl) {
            mLinkUrl = linkUrl;
            return this;
        }

        /**
         * Sets the source package name which requests the tab creation.
         */
        public Builder setSourcePackageName(String sourcePackageName) {
            mSourcePackageName = sourcePackageName;
            return this;
        }

        /** @return A fully constructed {@link BrowserActionsServiceIntentParams} object. */
        public BrowserActionsServiceIntentParams builder() {
            return new BrowserActionsServiceIntentParams(mAction, mLinkUrl, mSourcePackageName);
        }
    }
}
