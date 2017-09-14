// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;
import org.chromium.testing.local.LocalRobolectricTestRunner;
/**
 * Unit tests for {@link SuggestionsCarousel}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SuggestionsCarouselTest {
    public static final String URL_STRING = "http://www.test.com";

    @Mock
    private SuggestionsSource mSuggestionsSource;
    private SuggestionsCarousel mCarousel;
    private Context mContext;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        SuggestionsUiDelegate uiDelegate = mock(SuggestionsUiDelegate.class);
        when(uiDelegate.getSuggestionsSource()).thenReturn(mSuggestionsSource);

        mCarousel = new SuggestionsCarousel(
                mock(UiConfig.class), uiDelegate, mock(ContextMenuManager.class), null);
        mContext = RuntimeEnvironment.application.getApplicationContext();
    }

    @Test
    public void testEmptyUrl() {
        mCarousel.refresh(mContext, /* newURL = */ null);
        mCarousel.refresh(mContext, /* newURL = */ "");
        verify(mSuggestionsSource, never())
                .fetchContextualSuggestions(any(String.class), any(Callback.class));
    }

    @Test
    public void testValidUrlInitiatesSuggestionsFetch() {
        mCarousel.refresh(mContext, URL_STRING);
        verify(mSuggestionsSource)
                .fetchContextualSuggestions(any(String.class), any(Callback.class));
    }

    @Test
    public void testSameUrlDoesNotInitiateSecondFetch() {
        mCarousel.refresh(mContext, URL_STRING);
        mCarousel.refresh(mContext, URL_STRING);
        verify(mSuggestionsSource)
                .fetchContextualSuggestions(any(String.class), any(Callback.class));
    }

    @Test
    public void testDifferentUrlInitiatesSecondFetch() {
        mCarousel.refresh(mContext, URL_STRING);
        mCarousel.refresh(mContext, URL_STRING + "/foo");
        verify(mSuggestionsSource, times(2))
                .fetchContextualSuggestions(any(String.class), any(Callback.class));
    }
}
