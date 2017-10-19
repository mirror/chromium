// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.concurrent.Callable;

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
                new Callable<BookmarkModel>() {
                    @Override
                    public BookmarkModel call() throws Exception {
                        return new BookmarkModel();
                    }
                });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return bookmarkModel.isBookmarkModelLoaded();
            }
        });

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                bookmarkModel.destroy();
            }
        });
    }
}
