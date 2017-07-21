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

import org.chromium.base.Log;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

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

    // Released when thumbnail is generated (if necessary) and addEntryToDisked to cache.
    private static final Semaphore SEMAPHORE = new Semaphore(0);
    // Released when disk initialization (in an AsyncTask) is finished in setUp().
    private static final Semaphore INIT_SEMAPHORE = new Semaphore(0);

    private TestThumbnailProviderImpl mTestThumbnailProviderImpl;
    private TestThumbnailGenerator mTestThumbnailGenerator;
    private TestThumbnailDiskStorage mTestThumbnailDiskStorage;

    static class TestThumbnailRequest implements ThumbnailProvider.ThumbnailRequest {
        private String mContentId;
        private String mFilePath;

        public TestThumbnailRequest(String contentId, String filePath) {
            mContentId = contentId;
            mFilePath = filePath;
        }

        public String getFilePath() {
            return mFilePath;
        }

        public String getContentId() {
            return mContentId;
        }

        public void onThumbnailRetrieved(String filePath, Bitmap thumbnail) {}

        public int getIconSize() {
            return 0; // Not used in test
        }

        public boolean shouldCacheToDisk() {
            return true;
        }
    }

    static class TestThumbnailProviderImpl extends ThumbnailProviderImpl {
        public int retrievedCount;

        @Override
        public void onThumbnailRetrieved(String filePath, Bitmap bitmap) {
            ++retrievedCount;
        }
    }

    static class TestThumbnailDiskStorage extends ThumbnailDiskStorage {
        public int trimCount; // Number of times trim is called.

        class TestDiskWriteTask extends ThumbnailDiskStorage.DiskWriteTask {
            @Override
            protected Void doInBackground(Object... params) {
                super.doInBackground(params);
                SEMAPHORE.release();
                return null;
            }
        }

        public TestThumbnailDiskStorage(TestThumbnailGenerator thumbnailGenerator) {
            super(thumbnailGenerator);
        }

        @Override
        DiskWriteTask getWriteTask() {
            return new TestDiskWriteTask();
        }

        @Override
        void initDiskCache() {
            super.initDiskCache();
            INIT_SEMAPHORE.release();
        }

        @Override
        synchronized void trim() {
            super.trim();
            ++trimCount;
        }
    }

    /**
     * Dummy thumbnail generator that calls back immediately.
     */
    static class TestThumbnailGenerator extends ThumbnailGenerator {
        public int generateCount;

        @Override
        public void retrieveThumbnail(
                ThumbnailProvider.ThumbnailRequest request, ThumbnailGeneratorObserver observer) {
            ++generateCount;
            observer.onThumbnailRetrieved(null, null);
        }
    }

    @Before
    public void setUp() throws Exception {
        mTestThumbnailProviderImpl = new TestThumbnailProviderImpl();
        mTestThumbnailGenerator = new TestThumbnailGenerator();
        mTestThumbnailDiskStorage = new TestThumbnailDiskStorage(mTestThumbnailGenerator);
        try {
            INIT_SEMAPHORE.tryAcquire(10, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
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
        mTestThumbnailDiskStorage.retrieveThumbnail(request, mTestThumbnailProviderImpl);

        // Ensure thumbnail is cached (i.e. thumbnail generator is not called)
        assertGenerateCount(0);
        Assert.assertTrue(mTestThumbnailDiskStorage.get(CONTENT_ID1).sameAs(BITMAP1));

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
        Assert.assertTrue(mTestThumbnailDiskStorage.get(CONTENT_ID1).sameAs(BITMAP2));

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
        Assert.assertEquals(0, mTestThumbnailDiskStorage.trimCount);

        String filePath = mTestThumbnailDiskStorage.createFilePath(FILENAME1);
        TestThumbnailRequest request = new TestThumbnailRequest(CONTENT_ID1, filePath);
        mTestThumbnailDiskStorage.retrieveThumbnail(request, mTestThumbnailProviderImpl);
        assertGenerateCount(0);

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
        Assert.assertEquals(0, mTestThumbnailDiskStorage.trimCount);

        // addEntryToDisk one over limit
        mTestThumbnailDiskStorage.addEntryToDisk("contentId" + limit, BITMAP2, ICON_SIZE2);
        Assert.assertEquals(limit, mTestThumbnailDiskStorage.getCacheCount());

        // Verify oldest entry is trimmed
        Assert.assertEquals(1, mTestThumbnailDiskStorage.trimCount);
        Assert.assertEquals("contentId" + 1, mTestThumbnailDiskStorage.getOldestEntry().contentId);

        for (int i = 1; i <= limit; i++) {
            Assert.assertTrue(mTestThumbnailDiskStorage.remove("contentId" + i) != null);
        }
        Assert.assertEquals(0, mTestThumbnailDiskStorage.mSize);
    }

    /**
     * Assert number of times thumbnail generator is called after thumbnail has been
     * addEntryToDisked to cache.
     */
    private void assertGenerateCount(int expectedGenerateCount) {
        try {
            SEMAPHORE.tryAcquire(10, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }

        Assert.assertEquals(expectedGenerateCount, mTestThumbnailGenerator.generateCount);
    }

    private int dataSize() {
        return CONTENT_ID1.length() + Integer.BYTES
                + mTestThumbnailDiskStorage.createFilePath(CONTENT_ID1).length();
    }
}
