// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.partnerbookmarks;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.chrome.browser.DisableHistogramsRule;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for {@link PartnerBookmarksCache}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class PartnerBookmarksCacheTest {
    private static final String TEST_PREFERENCES_NAME = "partner_bookmarks_favicon_cache_test";

    private PartnerBookmarksCache mCache;

    @Rule
    public DisableHistogramsRule mDisableHistogramsRule = new DisableHistogramsRule();

    @Before
    public void setUp() throws Exception {
        mCache = new PartnerBookmarksCache(RuntimeEnvironment.application, TEST_PREFERENCES_NAME);
    }

    @After
    public void tearDown() throws Exception {
        mCache.clearCache();
    }

    @Test
    public void testInitEmptyCache() {
        Assert.assertEquals(mCache.numEntries(), 0);
    }

    @Test
    public void testCacheServerErrorFailures() {
        mCache.onFaviconFetched("url1", FaviconFetchResult.FAILURE_SERVER_ERROR);
        mCache.onFaviconFetched("url2", FaviconFetchResult.FAILURE_SERVER_ERROR);
        mCache.onFaviconFetched("url3", FaviconFetchResult.FAILURE_NOT_IN_CACHE);
        mCache.commit();

        mCache.init();
        Assert.assertFalse(mCache.shouldFetchFromServerIfNecessary("url1"));
        Assert.assertFalse(mCache.shouldFetchFromServerIfNecessary("url2"));
        Assert.assertTrue(mCache.shouldFetchFromServerIfNecessary("url3"));
    }

    @Test
    public void testOnlySuccessRemovesUrlFromCache() {
        mCache.onFaviconFetched("url1", FaviconFetchResult.FAILURE_SERVER_ERROR);
        mCache.commit();

        mCache.init();
        Assert.assertFalse(mCache.shouldFetchFromServerIfNecessary("url1"));
        mCache.onFaviconFetched("url1", FaviconFetchResult.FAILURE_ICON_SERVICE_UNAVAILABLE);
        mCache.commit();

        mCache.init();
        Assert.assertFalse(mCache.shouldFetchFromServerIfNecessary("url1"));
        mCache.onFaviconFetched("url1", FaviconFetchResult.FAILURE_NOT_IN_CACHE);
        mCache.commit();

        mCache.init();
        Assert.assertFalse(mCache.shouldFetchFromServerIfNecessary("url1"));
        mCache.onFaviconFetched("url1", FaviconFetchResult.FAILURE_CONNECTION_ERROR);
        mCache.commit();

        mCache.init();
        Assert.assertFalse(mCache.shouldFetchFromServerIfNecessary("url1"));
        mCache.onFaviconFetched("url1", FaviconFetchResult.SUCCESS);
        mCache.commit();

        mCache.init();
        Assert.assertTrue(mCache.shouldFetchFromServerIfNecessary("url1"));
    }

    @Test
    public void testShouldFetchFromServerIfNecessaryTrueIfUrlNotInCache() {
        Assert.assertTrue(mCache.shouldFetchFromServerIfNecessary("unused_url"));
    }

    // TODO(thildebr): Test the code path for #shouldFetchFromServerIfNecessary where the timeout
    //                 has expired. This requires mocking out System.currentTimeMillis() somehow.

    @Test
    public void testShouldFetchFromServerIfNecessaryFalseIfNotExpired() {
        mCache.onFaviconFetched("url1", FaviconFetchResult.FAILURE_SERVER_ERROR);
        mCache.commit();

        mCache.init();
        Assert.assertFalse(mCache.shouldFetchFromServerIfNecessary("url1"));
    }
}