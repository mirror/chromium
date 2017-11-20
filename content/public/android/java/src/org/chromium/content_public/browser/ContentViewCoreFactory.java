// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import android.content.Context;

import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.ContentViewCoreImpl;

/**
 * Factory class for {@link ContentViewCore}.
 */
public interface ContentViewCoreFactory {
    public static final ContentViewCoreFactory DEFAULT = (Context context,
            String productVersion) -> new ContentViewCoreImpl(context, productVersion);

    /**
     * @return {@link ContentViewCore} instance.
     */
    ContentViewCore create(Context context, String productVersion);
}
