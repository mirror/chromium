// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.spy;

import android.graphics.Bitmap;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.robolectric.annotation.Config;

import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.download.ui.ThumbnailProvider.ThumbnailRequest;
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
    private ThumbnailRequest mRequest;

    @Before
    public void setUp() {
        mRequest = new ThumbnailRequest() {
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
        mThumbnailProvider = spy(new ThumbnailProviderImpl(/* unused = */ 1));
    }

    @Test(expected = AssertionError.class)
    @Feature({"Ntp"})
    public void testTwoSidesBiggerThanRequiredSizeOfThumbnailRetrievedFails() {
        makeRequest(THUMBNAIL_SIZE_PX + 100, THUMBNAIL_SIZE_PX + 100);
    }

    @Test
    public void testWidthEqualsRequiredSizeOfThumbnailRetrievedSucceeds() {
        makeRequest(THUMBNAIL_SIZE_PX, THUMBNAIL_SIZE_PX + 100);
    }

    @Test
    public void testHeightEqualsRequiredSizeOfThumbnailRetrievedSucceeds() {
        makeRequest(THUMBNAIL_SIZE_PX + 100, THUMBNAIL_SIZE_PX);
    }

    @Test
    public void testTwoSidesSmallerThanRequiredSizeOfThumbnailRetrievedSucceeds() {
        makeRequest(THUMBNAIL_SIZE_PX - 100, THUMBNAIL_SIZE_PX - 100);
    }

    /**
     * Makes a request for a thumbnail to the thumbnail provider and overrides the native call to
     * return a fake thumbnail with dimensions width and height.
     * @param width The width of the retrieved fake thumbnail.
     * @param height The height of the retrieved fake thumbnail.
     */
    private void makeRequest(final int width, final int height) {
        doAnswer(new Answer() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Bitmap thumbnail = Bitmap.createBitmap(width, height, Bitmap.Config.ALPHA_8);
                mThumbnailProvider.onThumbnailRetrieved(FILE_PATH, thumbnail);
                return null;
            }
        })
                .when(mThumbnailProvider)
                .nativeRetrieveThumbnail(anyLong(), anyString(), anyInt());

        mThumbnailProvider.getThumbnail(mRequest);
    }
}
