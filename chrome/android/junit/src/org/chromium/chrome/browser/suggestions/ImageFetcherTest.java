// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import static org.chromium.chrome.test.util.browser.suggestions.ContentSuggestionsTestUtils.createDummySuggestion;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.DisableHistogramsRule;
import org.chromium.chrome.browser.download.ui.ThumbnailProvider;
import org.chromium.chrome.browser.download.ui.ThumbnailProvider.ThumbnailRequest;
import org.chromium.chrome.browser.favicon.FaviconHelper;
import org.chromium.chrome.browser.favicon.FaviconHelper.FaviconImageCallback;
import org.chromium.chrome.browser.favicon.LargeIconBridge;
import org.chromium.chrome.browser.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.chrome.browser.ntp.cards.CardsVariationParameters;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.ImageFetcher.DownloadThumbnailRequest;
import org.chromium.testing.local.LocalRobolectricTestRunner;

import java.util.HashMap;
/**
 * Unit tests for {@link ImageFetcher}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ImageFetcherTest {
    public static final int IMAGE_SIZE_PX = 100;
    public static final String URL = "www.test.com";
    @Rule
    public DisableHistogramsRule disableHistogramsRule = new DisableHistogramsRule();

    @Mock
    FaviconHelper mFaviconHelper;
    @Mock
    private SuggestionsUiDelegate mUiDelegate;
    @Mock
    private ThumbnailProvider mThumbnailProvider;
    @Mock
    private SuggestionsSource mSuggestionsSource;
    @Mock
    private LargeIconBridge mLargeIconBridge;

    private ImageFetcher mImageFetcher;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        CardsVariationParameters.setTestVariationParams(new HashMap<String, String>());
        mImageFetcher = spy(new ImageFetcher(mUiDelegate, mock(Profile.class)));

        doReturn(mFaviconHelper).when(mImageFetcher).getFaviconHelper();
        doReturn(mThumbnailProvider).when(mImageFetcher).getThumbnailProvider();
        doReturn(mLargeIconBridge).when(mImageFetcher).getLargeIconBridge();

        when(mUiDelegate.getSuggestionsSource()).thenReturn(mSuggestionsSource);
    }

    @Test
    public void testFaviconFetch() {
        SnippetArticle suggestion = createDummySuggestion(KnownCategories.BOOKMARKS);
        mImageFetcher.makeFaviconRequest(suggestion, IMAGE_SIZE_PX, mock(Callback.class));
        verify(mFaviconHelper)
                .getLocalFaviconImageForURL(
                        any(Profile.class), anyString(), anyInt(), any(FaviconImageCallback.class));
    }

    @Test
    public void testDownloadThumbnailFetch() {
        SnippetArticle suggestion = createDummySuggestion(KnownCategories.DOWNLOADS);
        DownloadThumbnailRequest request = mImageFetcher.makeDownloadThumbnailRequest(
                suggestion, IMAGE_SIZE_PX, mock(Callback.class));
        verify(mThumbnailProvider).getThumbnail(any(ThumbnailRequest.class));

        request.cancel();
        verify(mThumbnailProvider).cancelRetrieval(any(ThumbnailRequest.class));
    }

    @Test
    public void testArticleThumbnailFetch() {
        SnippetArticle suggestion = createDummySuggestion(KnownCategories.ARTICLES);
        mImageFetcher.makeArticleThumbnailRequest(suggestion, mock(Callback.class));
        verify(mUiDelegate.getSuggestionsSource())
                .fetchSuggestionImage(any(SnippetArticle.class), any(Callback.class));
    }

    @Test
    public void testLargeIconFetch() {
        mImageFetcher.makeLargeIconRequest(URL, IMAGE_SIZE_PX, mock(LargeIconCallback.class));
        verify(mLargeIconBridge)
                .getLargeIconForUrl(anyString(), anyInt(), any(LargeIconCallback.class));
    }
}
