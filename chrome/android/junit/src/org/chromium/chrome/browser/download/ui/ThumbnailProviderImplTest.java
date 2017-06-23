// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

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
        mRequest = mock(ThumbnailRequest.class);
        doReturn(FILE_PATH).when(mRequest).getFilePath();
        doReturn(THUMBNAIL_SIZE_PX).when(mRequest).getIconSize();

        // The native thumbnail provider will not be used, but needs to be != 0.
        mThumbnailProvider = spy(new ThumbnailProviderImpl(/* unused = */ 1));
    }

    @Test(expected = AssertionError.class)
    @Feature({"Ntp"})
    public void testTwoSidesBiggerThanRequiredSizeOfThumbnailRetrievedFails() {
        setUpProvider(THUMBNAIL_SIZE_PX + 100, THUMBNAIL_SIZE_PX + 100);
        mThumbnailProvider.getThumbnail(mRequest);
        verify(mRequest).onThumbnailRetrieved(anyString(), any(Bitmap.class));
    }

    @Test
    @Feature({"Ntp"})
    public void testWidthEqualsRequiredSizeOfThumbnailRetrievedSucceeds() {
        setUpProvider(THUMBNAIL_SIZE_PX, THUMBNAIL_SIZE_PX + 100);
        mThumbnailProvider.getThumbnail(mRequest);
        verify(mRequest).onThumbnailRetrieved(anyString(), any(Bitmap.class));
    }

    @Test
    @Feature({"Ntp"})
    public void testHeightEqualsRequiredSizeOfThumbnailRetrievedSucceeds() {
        setUpProvider(THUMBNAIL_SIZE_PX + 100, THUMBNAIL_SIZE_PX);
        mThumbnailProvider.getThumbnail(mRequest);
        verify(mRequest).onThumbnailRetrieved(anyString(), any(Bitmap.class));
    }

    @Test
    @Feature({"Ntp"})
    public void testTwoSidesSmallerThanRequiredSizeOfThumbnailRetrievedSucceeds() {
        setUpProvider(THUMBNAIL_SIZE_PX - 100, THUMBNAIL_SIZE_PX - 100);
        mThumbnailProvider.getThumbnail(mRequest);
        verify(mRequest).onThumbnailRetrieved(anyString(), any(Bitmap.class));
    }

    /**
     * Makes a request for a thumbnail to the thumbnail provider and overrides the native call to
     * return a fake thumbnail with dimensions width and height.
     * @param width The width of the retrieved fake thumbnail.
     * @param height The height of the retrieved fake thumbnail.
     */
    private void setUpProvider(final int width, final int height) {
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
    }
}
