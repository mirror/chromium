// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.partnerbookmarks;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.partnerbookmarks.PartnerBookmarksCache.CachedBookmark;

import java.util.Arrays;

/**
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class PartnerBookmarksCacheTest {
    private static final String TEST_DB_NAME = "pb_cache_test.db";
    private static final String TEST_URL = "testurl";
    private static final String TEST_URL_2 = "testurl2";

    private static final CachedBookmark BOOKMARK_1A = new CachedBookmark(TEST_URL, 100, 3);
    private static final CachedBookmark BOOKMARK_1B = new CachedBookmark(TEST_URL, 300, 4);
    private static final CachedBookmark BOOKMARK_2A = new CachedBookmark(TEST_URL_2, 100, 3);
    private static final CachedBookmark BOOKMARK_2B = new CachedBookmark(TEST_URL_2, 300, 4);

    private PartnerBookmarksCache mCache;

    @Before
    public void setUp() throws Exception {
        // Make sure we clean up any old databases just in case, so we get a fresh one every test.
        InstrumentationRegistry.getTargetContext().deleteDatabase(
                PartnerBookmarksCache.DATABASE_NAME);
        mCache =
                new PartnerBookmarksCache(InstrumentationRegistry.getTargetContext(), TEST_DB_NAME);
    }

    @After
    public void tearDown() throws Exception {
        InstrumentationRegistry.getTargetContext().deleteDatabase(
                PartnerBookmarksCache.DATABASE_NAME);
    }

    @Test
    @SmallTest
    @Feature({"PartnerBookmarksCache"})
    public void testCreateDb() {
        // Make sure we have an empty database when it's created.
        Assert.assertEquals(mCache.getBookmarksList().size(), 0);
    }

    @Test
    @SmallTest
    @Feature({"PartnerBookmarksCache"})
    public void testAddBookmark() {
        Assert.assertTrue(mCache.addBookmark(BOOKMARK_1A));
        Assert.assertEquals(mCache.getBookmarksList().size(), 1);
        Assert.assertNotNull(mCache.getBookmarkForUrl(TEST_URL));
    }

    @Test
    @SmallTest
    @Feature({"PartnerBookmarksCache"})
    public void testAddSameBookmark() {
        Assert.assertTrue(mCache.addBookmark(BOOKMARK_1A));
        Assert.assertFalse(mCache.addBookmark(BOOKMARK_1B));
        Assert.assertEquals(mCache.getBookmarksList().size(), 1);
    }

    @Test
    @SmallTest
    @Feature({"PartnerBookmarksCache"})
    public void testGetBookmarkForUrl() {
        Assert.assertTrue(mCache.addBookmark(BOOKMARK_1A));
        Assert.assertNotNull(mCache.getBookmarkForUrl(TEST_URL));
        Assert.assertNull(mCache.getBookmarkForUrl(TEST_URL_2));
    }

    @Test
    @SmallTest
    @Feature({"PartnerBookmarksCache"})
    public void testUpdateBookmark() {
        Assert.assertTrue(mCache.addBookmark(BOOKMARK_1A));
        Assert.assertTrue(mCache.addBookmark(BOOKMARK_2A));
        Assert.assertTrue(
                mCache.getBookmarksList().containsAll(Arrays.asList(BOOKMARK_1A, BOOKMARK_2A)));
        Assert.assertEquals(mCache.getBookmarksList().size(), 2);

        Assert.assertTrue(mCache.updateBookmark(BOOKMARK_1B));
        Assert.assertTrue(mCache.updateBookmark(BOOKMARK_2B));
        Assert.assertTrue(
                mCache.getBookmarksList().containsAll(Arrays.asList(BOOKMARK_1B, BOOKMARK_2B)));
        Assert.assertEquals(mCache.getBookmarksList().size(), 2);
    }
}