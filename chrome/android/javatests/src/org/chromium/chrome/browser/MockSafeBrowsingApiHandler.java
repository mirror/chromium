// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;

import org.chromium.base.ThreadUtils;
import org.chromium.components.safe_browsing.SafeBrowsingApiHandler;
import org.chromium.components.safe_browsing.SafeBrowsingApiHandler.Observer;

import java.util.HashMap;

/**
 * SafeBrowsingApiHandler that vends fake responses.
 */
class MockSafeBrowsingApiHandler implements SafeBrowsingApiHandler {
    private Observer mObserver;
    // Mock time it takes for a lookup request to complete.
    private static final long DEFAULT_CHECK_DELTA_US = 10;
    private static final String SAFE_METADATA = "{}";

    // Global url -> metadataResponse map. In practice there is only one SafeBrowsingApiHandler, but
    // it is cumbersome for tests to reach into the singleton instance directly. So just make this
    // static and modifiable from java tests using a static method.
    private static HashMap<String, String> sResponseMap = new HashMap<String, String>();

    @Override
    public boolean init(Context context, Observer observer) {
        mObserver = observer;
        return true;
    }

    @Override
    public void startUriLookup(final long callbackId, String uri, int[] threatsOfInterest) {
        final String metadata;
        if (sResponseMap.containsKey(uri)) {
            metadata = sResponseMap.get(uri);
        } else {
            metadata = SAFE_METADATA;
        }

        // clang-format off
      ThreadUtils.runOnUiThread(
          (Runnable) () -> mObserver.onUrlCheckDone(
              callbackId, STATUS_SUCCESS, metadata, DEFAULT_CHECK_DELTA_US));
        // clang-format on
    }

    public static void addMockResponse(String uri, String metadata) {
        sResponseMap.put(uri, metadata);
    }

    public static void clearMockResponses() {
        sResponseMap.clear();
    }
}
