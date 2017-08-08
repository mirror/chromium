// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util;

import junit.framework.Assert;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;

import java.util.concurrent.TimeoutException;

/**
 * Utility functions for dealing with bookmarks in tests.
 */
public class BookmarkTestUtil {

    /**
     * Waits until the bookmark model is loaded, i.e. until
     * {@link BookmarkModel#isBookmarkModelLoaded()} is true.
     */
    public static void waitForBookmarkModelLoaded() throws InterruptedException {
        final BookmarkModel bookmarkModel = ThreadUtils.runOnUiThreadBlockingNoException(
                () -> new BookmarkModel());

        final CallbackHelper loadedCallback = new CallbackHelper();
        ThreadUtils.runOnUiThreadBlocking(
                (Runnable) () -> bookmarkModel.runAfterBookmarkModelLoaded(
                        () -> loadedCallback.notifyCalled()));

        try {
            loadedCallback.waitForCallback(0);
        } catch (TimeoutException e) {
            Assert.fail("Bookmark model did not load: Timeout.");
        }

        ThreadUtils.runOnUiThreadBlocking(() -> bookmarkModel.destroy());
    }
}
