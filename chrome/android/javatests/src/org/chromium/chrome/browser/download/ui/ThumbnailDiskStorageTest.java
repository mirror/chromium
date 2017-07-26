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

import java.util.concurrent.atomic.AtomicBoolean;

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

    private static final long TIMEOUT_MS = 10000;
    private static final long INTERVAL_MS = 500;

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
        public AtomicBoolean initialized = new AtomicBoolean();

        public TestThumbnailDiskStorage(
                TestThumbnailProviderImpl requester, TestThumbnailGenerator thumbnailGenerator) {
            super(requester, thumbnailGenerator);
        }

        @Override
        void initDiskCache() {
            super.initDiskCache();
            initialized.set(true);
        }

        @Override
        public boolean removeFromDisk(String contentId) {
            ++removeCount;
            return super.removeFromDisk(contentId);
        }

        /**
         * The number of entries in the disk cache. Accessed in testing thread.
         */
        int getCacheCount() {
            return sDiskLruCache.size();
        }

        public String getOldestContentId() {
            if (getCacheCount() <= 0) return null;

            return sDiskLruCache.iterator().next();
        }

        public String getMostRecentContentId() {
            if (getCacheCount() <= 0) return null;

            String[] setArray = sDiskLruCache.toArray(new String[sDiskLruCache.size()]);
            return setArray[sDiskLruCache.size() - 1];
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
        assertInitialized();
    }

    /**
     * Verify that an inserted thumbnail can be retrieved.
     */
    @Test
    @SmallTest
    public void testCanInsertAndGet() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);

        mTestThumbnailDiskStorage.addToDisk(CONTENT_ID1, BITMAP1);
        Assert.assertEquals(1, mTestThumbnailDiskStorage.getCacheCount());

        String filePath = mTestThumbnailDiskStorage.mDirectory.getPath() + FILENAME1;
        TestThumbnailRequest request = new TestThumbnailRequest(CONTENT_ID1, filePath);
        mTestThumbnailDiskStorage.retrieveThumbnail(request);

        // Ensure the thumbnail is retrieved and the thumbnail generator is not called.
        assertThumbnailRetrieved();
        Assert.assertEquals(mTestThumbnailGenerator.generateCount, 0);
        Assert.assertTrue(mTestThumbnailDiskStorage.getFromDisk(CONTENT_ID1).sameAs(BITMAP1));

        // Clean up
        Assert.assertTrue(mTestThumbnailDiskStorage.removeFromDisk(CONTENT_ID1));
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

        mTestThumbnailDiskStorage.addToDisk(CONTENT_ID1, BITMAP1);
        mTestThumbnailDiskStorage.addToDisk(CONTENT_ID1, BITMAP2);

        // Verify that the old entry is updated with the new
        Assert.assertEquals(1, mTestThumbnailDiskStorage.getCacheCount());
        Assert.assertTrue(mTestThumbnailDiskStorage.getFromDisk(CONTENT_ID1).sameAs(BITMAP2));

        Assert.assertTrue(mTestThumbnailDiskStorage.removeFromDisk(CONTENT_ID1));
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);
    }

    /**
     * Verify that retrieveThumbnail makes the called entry the most recent entry in cache.
     */
    @Test
    @SmallTest
    public void testRetrieveThumbnailShouldMakeEntryMostRecent() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);

        mTestThumbnailDiskStorage.addToDisk(CONTENT_ID1, BITMAP1);
        mTestThumbnailDiskStorage.addToDisk(CONTENT_ID2, BITMAP1);
        mTestThumbnailDiskStorage.addToDisk(CONTENT_ID3, BITMAP1);
        Assert.assertEquals(3, mTestThumbnailDiskStorage.getCacheCount());

        // Verify no trimming is done
        Assert.assertEquals(0, mTestThumbnailDiskStorage.removeCount);

        String filePath = mTestThumbnailDiskStorage.getThumbnailFilePath(FILENAME1);
        TestThumbnailRequest request = new TestThumbnailRequest(CONTENT_ID1, filePath);
        mTestThumbnailDiskStorage.retrieveThumbnail(request);
        assertThumbnailRetrieved();
        Assert.assertEquals(mTestThumbnailGenerator.generateCount, 0);

        // Verify that the called entry is the most recent entry
        Assert.assertEquals(CONTENT_ID1, mTestThumbnailDiskStorage.getMostRecentContentId());

        Assert.assertTrue(mTestThumbnailDiskStorage.removeFromDisk(CONTENT_ID1));
        Assert.assertTrue(mTestThumbnailDiskStorage.removeFromDisk(CONTENT_ID2));
        Assert.assertTrue(mTestThumbnailDiskStorage.removeFromDisk(CONTENT_ID3));
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);
    }

    /**
     * Verify that trim removes the least recently used entry.
     */
    @Test
    @SmallTest
    public void testExceedLimitShouldTrim() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);

        // Add thumbnails up to cache limit to get 1 entry trimmed
        int count = 0;
        while (mTestThumbnailDiskStorage.removeCount == 0) {
            String filename = "pic" + count + ".png";
            mTestThumbnailDiskStorage.addToDisk("contentId" + count, BITMAP2);
            ++count;
        }

        // Since count includes the oldest entry trimmed, verify that cache size is one less
        Assert.assertEquals(count - 1, mTestThumbnailDiskStorage.getCacheCount());
        // Verify oldest entry is trimmed (i.e. previously second-oldest entry is now the oldest)
        Assert.assertEquals("contentId" + 1, mTestThumbnailDiskStorage.getOldestContentId());

        for (int i = 1; i <= count - 1; i++) {
            Assert.assertTrue(mTestThumbnailDiskStorage.removeFromDisk("contentId" + i));
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
        }, TIMEOUT_MS, INTERVAL_MS);
    }

    private void assertInitialized() throws Exception {
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mTestThumbnailDiskStorage.initialized.get();
            }
        }, TIMEOUT_MS, INTERVAL_MS);
    }

    private int dataSize() {
        return CONTENT_ID1.length() + Integer.BYTES;
    }
}
