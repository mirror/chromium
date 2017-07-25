// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.graphics.Bitmap;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.ArrayList;
import java.util.Map;

/**
 * Tests ThumbnailProviderDiskStorage.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class ThumbnailDiskStorageTest {
    private static final String TAG = "ThumbnailDiskTest";
    private static final String CONTENT_ID1 = "contentId1";
    private static final String CONTENT_ID2 = "contentId2";
    private static final String CONTENT_ID3 = "contentId3";
    private static final String FILENAME1 = "pic1.png";
    private static final String FILENAME2 = "pic2.png";
    private static final String FILENAME3 = "pic3.png";
    private static final Bitmap BITMAP1 = Bitmap.createBitmap(64, 64, Bitmap.Config.ARGB_8888);
    private static final Bitmap BITMAP2 = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    private static final int ICON_SIZE1 = BITMAP1.getByteCount();
    private static final int ICON_SIZE2 = BITMAP2.getByteCount();

    private static final long MS_TIMEOUT = 10000;
    private static final long MS_INTERVAL = 500;

    private TestThumbnailProviderImpl mTestThumbnailProviderImpl;
    private TestThumbnailGenerator mTestThumbnailGenerator;
    private TestThumbnailDiskStorage mTestThumbnailDiskStorage;

    private static class TestThumbnailRequest implements ThumbnailProvider.ThumbnailRequest {
        private String mContentId;
        private String mFilePath;

        public TestThumbnailRequest(String contentId, String filePath) {
            mContentId = contentId;
            mFilePath = filePath;
        }

        @Override
        public String getFilePath() {
            return mFilePath;
        }

        @Override
        public String getContentId() {
            return mContentId;
        }

        @Override
        public void onThumbnailRetrieved(String filePath, Bitmap thumbnail) {}

        @Override
        public int getIconSize() {
            return 0; // Not used in test
        }

        @Override
        public boolean shouldCacheToDisk() {
            return true;
        }
    }

    private static class TestThumbnailProviderImpl extends ThumbnailProviderImpl {
        public int retrievedCount;

        @Override
        public void onThumbnailRetrieved(String contentId, Bitmap bitmap) {
            ++retrievedCount;
        }
    }

    private static class TestThumbnailDiskStorage extends ThumbnailDiskStorage {
        // Number of removes. Incremented in adding an existing entry and trimming.
        public int removeCount;
        public boolean initialized;

        public TestThumbnailDiskStorage(
                TestThumbnailProviderImpl requester, TestThumbnailGenerator thumbnailGenerator) {
            super(requester, thumbnailGenerator);
        }

        @Override
        void initDiskCache() {
            super.initDiskCache();
            initialized = true;
        }

        @Override
        ThumbnailEntry remove(String contentId) {
            ++removeCount;
            return super.remove(contentId);
        }

        public ThumbnailEntry getOldestEntry() {
            if (getCacheCount() <= 0) return null;

            return sDiskLruCache.entrySet().iterator().next().getValue();
        }

        public ThumbnailEntry getMostRecentEntry() {
            if (getCacheCount() <= 0) return null;

            ArrayList<Map.Entry<String, ThumbnailEntry>> entryList =
                    new ArrayList<Map.Entry<String, ThumbnailEntry>>(sDiskLruCache.entrySet());
            return entryList.get(entryList.size() - 1).getValue();
        }
    }

    /**
     * Dummy thumbnail generator that calls back immediately.
     */
    private static class TestThumbnailGenerator extends ThumbnailGenerator {
        public int generateCount;

        @Override
        public void retrieveThumbnail(ThumbnailProvider.ThumbnailRequest request) {
            ++generateCount;
            onThumbnailRetrieved(request.getContentId(), request.getIconSize(), null, true);
        }
    }

    @Before
    public void setUp() throws Exception {
        mTestThumbnailProviderImpl = new TestThumbnailProviderImpl();
        mTestThumbnailGenerator = new TestThumbnailGenerator();
        mTestThumbnailDiskStorage =
                new TestThumbnailDiskStorage(mTestThumbnailProviderImpl, mTestThumbnailGenerator);
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mTestThumbnailDiskStorage.initialized;
            }
        }, MS_TIMEOUT, MS_INTERVAL);
    }

    /**
     * Verify that an inserted thumbnail can be retrieved.
     */
    @Test
    @SmallTest
    public void testCanInsertAndGet() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);

        mTestThumbnailDiskStorage.addEntryToDisk(CONTENT_ID1, BITMAP1, ICON_SIZE1);
        Assert.assertEquals(ICON_SIZE1 + dataSize(), mTestThumbnailDiskStorage.mSize);

        String filePath = mTestThumbnailDiskStorage.mDirectory.getPath() + FILENAME1;
        TestThumbnailRequest request = new TestThumbnailRequest(CONTENT_ID1, filePath);
        mTestThumbnailDiskStorage.retrieveThumbnail(request);

        // Ensure the retrieved thumbnail was cached (i.e. thumbnail generator is not called)
        assertThumbnailRetrieved();
        Assert.assertEquals(mTestThumbnailGenerator.generateCount, 0);
        Assert.assertTrue(mTestThumbnailDiskStorage.getFromDisk(CONTENT_ID1).sameAs(BITMAP1));

        // Clean up
        Assert.assertTrue(mTestThumbnailDiskStorage.remove(CONTENT_ID1) != null);
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);
    }

    /**
     * Verify that two inserted entries with the same key (content ID) will count as only one entry
     * and the first entry data will be replaced with the second.
     */
    @Test
    @SmallTest
    public void testRepeatedInsertShouldBeUpdated() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);

        mTestThumbnailDiskStorage.addEntryToDisk(CONTENT_ID1, BITMAP1, ICON_SIZE1);
        mTestThumbnailDiskStorage.addEntryToDisk(CONTENT_ID1, BITMAP2, ICON_SIZE2);

        // Verify that the old entry is updated with the new
        Assert.assertEquals(ICON_SIZE2 + dataSize(), mTestThumbnailDiskStorage.mSize);
        Assert.assertTrue(mTestThumbnailDiskStorage.getFromDisk(CONTENT_ID1).sameAs(BITMAP2));

        Assert.assertTrue(mTestThumbnailDiskStorage.remove(CONTENT_ID1) != null);
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);
    }

    /**
     * Verify that retrieveThumbnail makes the called entry the most recent entry in cache.
     */
    @Test
    @SmallTest
    public void testRetrieveThumbnailShouldMakeEntryMostRecent() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);

        mTestThumbnailDiskStorage.addEntryToDisk(CONTENT_ID1, BITMAP1, ICON_SIZE1);
        mTestThumbnailDiskStorage.addEntryToDisk(CONTENT_ID2, BITMAP1, ICON_SIZE1);
        mTestThumbnailDiskStorage.addEntryToDisk(CONTENT_ID3, BITMAP1, ICON_SIZE1);
        Assert.assertEquals((ICON_SIZE1 + dataSize()) * 3, mTestThumbnailDiskStorage.mSize);

        // Verify no trimming is done
        Assert.assertEquals(0, mTestThumbnailDiskStorage.removeCount);

        String filePath = mTestThumbnailDiskStorage.createFilePath(FILENAME1);
        TestThumbnailRequest request = new TestThumbnailRequest(CONTENT_ID1, filePath);
        mTestThumbnailDiskStorage.retrieveThumbnail(request);
        assertThumbnailRetrieved();
        Assert.assertEquals(mTestThumbnailGenerator.generateCount, 0);

        // Verify that the called entry is the most recent entry
        Assert.assertEquals(CONTENT_ID1, mTestThumbnailDiskStorage.getMostRecentEntry().contentId);

        Assert.assertTrue(mTestThumbnailDiskStorage.remove(CONTENT_ID1) != null);
        Assert.assertTrue(mTestThumbnailDiskStorage.remove(CONTENT_ID2) != null);
        Assert.assertTrue(mTestThumbnailDiskStorage.remove(CONTENT_ID3) != null);
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);
    }

    /**
     * Verify that trim is called when the size is over limit.
     */
    @Test
    @SmallTest
    public void testExceedLimitShouldTrim() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);

        // addEntryToDisk thumbnails up to cache limit
        int limit = 1024 * 1024 / 40000;
        for (int i = 0; i < limit; i++) {
            String filename = "pic" + i + ".png";
            mTestThumbnailDiskStorage.addEntryToDisk("contentId" + i, BITMAP2, ICON_SIZE2);
        }

        // Verify no trimming is done
        Assert.assertEquals(0, mTestThumbnailDiskStorage.removeCount);

        // addEntryToDisk one over limit
        mTestThumbnailDiskStorage.addEntryToDisk("contentId" + limit, BITMAP2, ICON_SIZE2);
        Assert.assertEquals(limit, mTestThumbnailDiskStorage.getCacheCount());

        // Verify oldest entry is trimmed
        Assert.assertEquals(1, mTestThumbnailDiskStorage.removeCount);
        Assert.assertEquals("contentId" + 1, mTestThumbnailDiskStorage.getOldestEntry().contentId);

        for (int i = 1; i <= limit; i++) {
            Assert.assertTrue(mTestThumbnailDiskStorage.remove("contentId" + i) != null);
        }
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);
    }

    /**
     * Assert that {@link ThumbnailProviderImpl} has retrieved the requested thumbnail.
     */
    private void assertThumbnailRetrieved() throws Exception {
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mTestThumbnailProviderImpl.retrievedCount == 1;
            }
        }, MS_TIMEOUT, MS_INTERVAL);
    }

    private int dataSize() {
        return CONTENT_ID1.length() + Integer.BYTES
                + mTestThumbnailDiskStorage.createFilePath(CONTENT_ID1).length();
    }
}
