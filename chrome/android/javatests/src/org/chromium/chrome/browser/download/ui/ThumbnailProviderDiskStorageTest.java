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
public class ThumbnailProviderDiskStorageTest {
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

    // Released when thumbnail is generated (if necessary) and added to cache.
    private static final Semaphore SEMAPHORE = new Semaphore(0);

    private TestThumbnailProviderImpl mTestThumbnailProviderImpl;
    private TestThumbnailProviderGenerator mTestThumbnailProviderGenerator;
    private TestThumbnailProviderDiskStorage mTestThumbnailProviderDiskStorage;

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
    }

    static class TestThumbnailProviderImpl extends ThumbnailProviderImpl {
        public int retrievedCount;

        @Override
        public void onThumbnailRetrieved(String filePath, Bitmap bitmap) {
            ++retrievedCount;
        }
    }

    static class TestThumbnailProviderDiskStorage extends ThumbnailProviderDiskStorage {
        public int trimCount; // Number of times trim is called.

        class TestDiskWriteTask extends ThumbnailProviderDiskStorage.DiskWriteTask {
            @Override
            protected Void doInBackground(Object... params) {
                super.doInBackground(params);
                SEMAPHORE.release();
                return null;
            }
        }

        public TestThumbnailProviderDiskStorage(TestThumbnailProviderGenerator thumbnailGenerator) {
            super(thumbnailGenerator);
        }

        @Override
        DiskWriteTask getWriteTask() {
            return new TestDiskWriteTask();
        }

        @Override
        synchronized void trim() {
            super.trim();
            ++trimCount;
        }
    }

    /**
     * Dummy thumbnail generator that calls back immediately.
     * */
    static class TestThumbnailProviderGenerator extends ThumbnailProviderGenerator {
        public int generateCount;

        @Override
        public void retrieveThumbnail(ThumbnailProvider.ThumbnailRequest request,
                ThumbnailProviderGeneratorObserver observer) {
            ++generateCount;
            observer.onThumbnailRetrieved(null, null);
        }
    }

    @Before
    public void setUp() throws Exception {
        mTestThumbnailProviderImpl = new TestThumbnailProviderImpl();
        mTestThumbnailProviderGenerator = new TestThumbnailProviderGenerator();
        mTestThumbnailProviderDiskStorage =
                new TestThumbnailProviderDiskStorage(mTestThumbnailProviderGenerator);
    }

    /**
     * Verify that an inserted thumbnail can be retrieved.
     */
    @Test
    @SmallTest
    public void testCanInsertAndGet() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.mSize);

        String filePath = mTestThumbnailProviderDiskStorage.mDirectory.getPath() + FILENAME1;
        mTestThumbnailProviderDiskStorage.add(CONTENT_ID1, BITMAP1, ICON_SIZE1, filePath);
        Assert.assertEquals(ICON_SIZE1 + dataSize(), mTestThumbnailProviderDiskStorage.mSize);

        TestThumbnailRequest request = new TestThumbnailRequest(CONTENT_ID1, "");
        mTestThumbnailProviderDiskStorage.retrieveThumbnail(request, mTestThumbnailProviderImpl);

        // Ensure thumbnail is cached (i.e. thumbnail generator is not called)
        assertGenerateCount(0);

        // Clean up
        Assert.assertTrue(mTestThumbnailProviderDiskStorage.remove(CONTENT_ID1) != null);
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.mSize);
    }

    /**
     * Verify that two inserted entries with the same key (content ID) will count as only one entry
     * and the first entry data will be replaced with the second.
     */
    @Test
    @SmallTest
    public void testRepeatedInsertShouldBeUpdated() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.mSize);

        String filePath = mTestThumbnailProviderDiskStorage.mDirectory.getPath() + FILENAME1;
        mTestThumbnailProviderDiskStorage.add(CONTENT_ID1, BITMAP1, ICON_SIZE1, filePath);
        mTestThumbnailProviderDiskStorage.add(CONTENT_ID1, BITMAP2, ICON_SIZE2, filePath);

        // Verify that the old entry is updated with the new
        Assert.assertEquals(ICON_SIZE2 + dataSize(), mTestThumbnailProviderDiskStorage.mSize);
        Assert.assertTrue(mTestThumbnailProviderDiskStorage.get(CONTENT_ID1).sameAs(BITMAP2));

        Assert.assertTrue(mTestThumbnailProviderDiskStorage.remove(CONTENT_ID1) != null);
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.mSize);
    }

    /**
     * Verify that retrieveThumbnail makes the called entry the most recent entry in cache.
     */
    @Test
    @SmallTest
    public void testRetrieveThumbnailShouldMakeEntryMostRecent() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.mSize);

        String filePath = mTestThumbnailProviderDiskStorage.createFilePath(FILENAME1);
        String filePath2 = mTestThumbnailProviderDiskStorage.createFilePath(FILENAME2);
        String filePath3 = mTestThumbnailProviderDiskStorage.createFilePath(FILENAME3);
        mTestThumbnailProviderDiskStorage.add(CONTENT_ID1, BITMAP1, ICON_SIZE1, filePath);
        mTestThumbnailProviderDiskStorage.add(CONTENT_ID2, BITMAP1, ICON_SIZE1, filePath2);
        mTestThumbnailProviderDiskStorage.add(CONTENT_ID3, BITMAP1, ICON_SIZE1, filePath3);
        Assert.assertEquals((ICON_SIZE1 + dataSize()) * 3, mTestThumbnailProviderDiskStorage.mSize);

        // Verify no trimming is done
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.trimCount);

        TestThumbnailRequest request = new TestThumbnailRequest(CONTENT_ID1, filePath);
        mTestThumbnailProviderDiskStorage.retrieveThumbnail(request, mTestThumbnailProviderImpl);
        assertGenerateCount(0);

        // Verify that the called entry is the most recent entry
        Assert.assertEquals(
                CONTENT_ID1, mTestThumbnailProviderDiskStorage.getMostRecentEntry().contentId);

        Assert.assertTrue(mTestThumbnailProviderDiskStorage.remove(CONTENT_ID1) != null);
        Assert.assertTrue(mTestThumbnailProviderDiskStorage.remove(CONTENT_ID2) != null);
        Assert.assertTrue(mTestThumbnailProviderDiskStorage.remove(CONTENT_ID3) != null);
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.mSize);
    }

    /**
     * Verify that trim is called when the size is over limit.
     */
    @Test
    @SmallTest
    public void testExceedLimitShouldTrim() throws Throwable {
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.mSize);

        // Add thumbnails up to cache limit
        int limit = 1024 * 1024 / 40000;
        for (int i = 0; i < limit; i++) {
            String filename = "pic" + i + ".png";
            String filePath = mTestThumbnailProviderDiskStorage.createFilePath(filename);
            mTestThumbnailProviderDiskStorage.add("contentId" + i, BITMAP2, ICON_SIZE2, filePath);
        }

        // Verify no trimming is done
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.trimCount);

        // Add one over limit
        String filePath = mTestThumbnailProviderDiskStorage.createFilePath("pic" + limit + ".png");
        mTestThumbnailProviderDiskStorage.add("contentId" + limit, BITMAP2, ICON_SIZE2, filePath);
        Assert.assertEquals(limit, mTestThumbnailProviderDiskStorage.getCacheCount());
        Assert.assertEquals(1, mTestThumbnailProviderDiskStorage.trimCount);
        Assert.assertEquals(
                "contentId" + 1, mTestThumbnailProviderDiskStorage.getOldestEntry().contentId);

        for (int i = 1; i <= limit; i++) {
            Assert.assertTrue(mTestThumbnailProviderDiskStorage.remove("contentId" + i) != null);
        }
        Assert.assertEquals(0, mTestThumbnailProviderDiskStorage.mSize);
    }

    /**
     * Assert number of times thumbnail generator is called after thumbnail has been added to cache.
     */
    private void assertGenerateCount(int expectedGenerateCount) {
        try {
            SEMAPHORE.tryAcquire(10, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }

        Assert.assertEquals(expectedGenerateCount, mTestThumbnailProviderGenerator.generateCount);
    }

    private int dataSize() {
        return CONTENT_ID1.length() + Integer.BYTES
                + mTestThumbnailProviderDiskStorage.createFilePath(CONTENT_ID1).length();
    }
}
