// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;

import android.graphics.Bitmap;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.util.Feature;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for {@link ThumbnailProvider}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ThumbnailProviderImplTest {
    private static final String FILE_PATH = "path/to/file";
    public static final int THUMBNAIL_SIZE_PX = 200;
    private ThumbnailProviderImpl mThumbnailProvider;

    @Before
    public void setUp() {
        ThumbnailProvider.ThumbnailRequest request = new ThumbnailProvider.ThumbnailRequest() {
            @Override
            public String getFilePath() {
                return FILE_PATH;
            }

            @Override
            public void onThumbnailRetrieved(String filePath, Bitmap thumbnail) {
                // Do nothing.
            }

            @Override
            public int getIconSize() {
                return THUMBNAIL_SIZE_PX;
            }
        };

        // The native thumbnail provider will not be used, but needs to be != 0.
        mThumbnailProvider = spy(new ThumbnailProviderImpl(/* nativeThumbnailProvider = */ 1));
        doNothing().when(mThumbnailProvider).retrieveThumbnail();
        doReturn(null).when(mThumbnailProvider).getBitmapFromCache(anyString(), anyInt());

        mThumbnailProvider.getThumbnail(request);
    }

    @Test(expected = AssertionError.class)
    @Feature({"Ntp"})
    public void testTwoSidesBiggerThanRequiredSizeOfThumbnailRetrievedFails() {
        Bitmap bitmap = Bitmap.createBitmap(300, 300, Bitmap.Config.ALPHA_8);
        mThumbnailProvider.onThumbnailRetrieved(FILE_PATH, bitmap);
    }

    @Test
    public void testWidthEqualsRequiredSizeOfThumbnailRetrievedSucceeds() {
        Bitmap bitmap = Bitmap.createBitmap(200, 300, Bitmap.Config.ALPHA_8);
        mThumbnailProvider.onThumbnailRetrieved(FILE_PATH, bitmap);
    }

    @Test
    public void testHeightEqualsRequiredSizeOfThumbnailRetrievedSucceeds() {
        Bitmap bitmap = Bitmap.createBitmap(300, 200, Bitmap.Config.ALPHA_8);
        mThumbnailProvider.onThumbnailRetrieved(FILE_PATH, bitmap);
    }

    @Test
    public void testTwoSidesSmallerThanRequiredSizeOfThumbnailRetrievedSucceeds() {
        Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ALPHA_8);
        mThumbnailProvider.onThumbnailRetrieved(FILE_PATH, bitmap);
    }
}
