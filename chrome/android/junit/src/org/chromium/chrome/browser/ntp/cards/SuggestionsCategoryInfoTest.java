// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import static org.hamcrest.CoreMatchers.is;
import static org.hamcrest.CoreMatchers.nullValue;
import static org.junit.Assert.assertThat;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.chrome.test.util.browser.suggestions.ContentSuggestionsTestUtils.CategoryInfoBuilder;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for {@link SuggestionsCategoryInfo}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
@DisableFeatures(ChromeFeatureList.CHROME_HOME)
public class SuggestionsCategoryInfoTest {
    @Rule
    public TestRule mRule = new Features.JUnitProcessor();

    @Test
    @EnableFeatures(
            {ChromeFeatureList.CHROME_HOME, ChromeFeatureList.CHROME_HOME_DESTROY_SUGGESTIONS})
    public void
    testDownloadContextMenu() {
        SuggestionsCategoryInfo categoryInfo =
                new CategoryInfoBuilder(KnownCategories.DOWNLOADS).build();
        assertThat(
                categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_OPEN_IN_NEW_WINDOW),
                is(true));
        assertThat(categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_OPEN_IN_NEW_TAB),
                is(true));
        assertThat(categoryInfo.isContextMenuItemSupported(
                           ContextMenuManager.ID_OPEN_IN_INCOGNITO_TAB),
                is(false));
        assertThat(categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_SAVE_FOR_OFFLINE),
                is(false));
        assertThat(
                categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_REMOVE), nullValue());
    }

    @Test
    @EnableFeatures(ChromeFeatureList.ALLOW_READER_FOR_ACCESSIBILITY)
    @DisableFeatures(
            {ChromeFeatureList.ANDROID_PAY_INTEGRATION_V1, ChromeFeatureList.ANDROID_PAYMENT_APPS})
    public void
    testRecentTabContextMenu() {
        SuggestionsCategoryInfo categoryInfo =
                new CategoryInfoBuilder(KnownCategories.RECENT_TABS).build();
        assertThat(
                categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_OPEN_IN_NEW_WINDOW),
                is(false));
        assertThat(categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_OPEN_IN_NEW_TAB),
                is(false));
        assertThat(categoryInfo.isContextMenuItemSupported(
                           ContextMenuManager.ID_OPEN_IN_INCOGNITO_TAB),
                is(false));
        assertThat(categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_SAVE_FOR_OFFLINE),
                is(false));
        assertThat(
                categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_REMOVE), nullValue());
    }

    @Test
    public void testArticleContextMenu() {
        SuggestionsCategoryInfo categoryInfo =
                new CategoryInfoBuilder(KnownCategories.ARTICLES).build();
        assertThat(
                categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_OPEN_IN_NEW_WINDOW),
                is(true));
        assertThat(categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_OPEN_IN_NEW_TAB),
                is(true));
        assertThat(categoryInfo.isContextMenuItemSupported(
                           ContextMenuManager.ID_OPEN_IN_INCOGNITO_TAB),
                is(true));
        assertThat(categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_SAVE_FOR_OFFLINE),
                is(true));
        assertThat(
                categoryInfo.isContextMenuItemSupported(ContextMenuManager.ID_REMOVE), nullValue());
    }
}
