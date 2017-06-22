// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import org.junit.Assert;
import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.ntp.snippets.KnownCategories;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.suggestions.SuggestionsEventReporterBridge;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegateImpl;
import org.chromium.chrome.test.util.browser.suggestions.ContentSuggestionsTestUtils;
import org.chromium.chrome.test.util.browser.suggestions.DummySuggestionsEventReporter;
import org.chromium.chrome.test.util.browser.suggestions.FakeSuggestionsSource;

import java.util.Arrays;
import java.util.Calendar;

/**
 * A canned, inflexible, set of pretty suggestions for use in Ui cataloging tests.
 */
public class UiCaptureSuggestionsRule extends TestWatcher {
    private static final SnippetArticle[] FAKE_ARTICLE_SUGGESTIONS = new SnippetArticle[] {

            new SnippetArticle(KnownCategories.ARTICLES, "suggestion0",
                    "Lorem ipsum dolor sit amet, consectetur ", "Lorem Ipsum",
                    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
                            + "tempor incididunt ut labore et dolore magna aliqua.",
                    "junk1", getTimestamp(2017, Calendar.JUNE, 1), 0.0f, 0L),
            new SnippetArticle(KnownCategories.ARTICLES, "suggestion1",
                    "A title that just goes on, and on, and on; tells a complete story at great "
                            + "length, and is simply too long.",
                    "A publisher with a very long name", "preview", "junk1",
                    getTimestamp(2017, Calendar.JANUARY, 30), 0.0f, 0L)};
    public static final int[] CATEGORIES = {KnownCategories.ARTICLES, KnownCategories.BOOKMARKS};

    private static final SnippetArticle[] FAKE_BOOKMARK_SUGGESTIONS = new SnippetArticle[] {
            new SnippetArticle(KnownCategories.BOOKMARKS, "suggestion2",
                    "Look, it’s very, very simple…. All I want… is a cup of tea.", "Publisher",
                    "My attempt to get a teapot and kettle in London (y'know that country that "
                            + "drinks staggering amounts of tea) ... should have been easy, "
                            + "right?\n"
                            + "\n"
                            + "The ticket includes this gem:\n"
                            + "--------\n"
                            + " The concern with kettles is the fact that they are portable and "
                            + "carry hot water.\n"
                            + "\n"
                            + "Yes, they are designed explicitly to do that, safely.\n"
                            + "\n"
                            + " There is a concern that staff are at risk to burns and scalds "
                            + "when combining the two. \n"
                            + "\n"
                            + "Unlike the large machine that puts out pressurized steam ?\n",
                    "junk1", getTimestamp(2017, Calendar.MARCH, 10), 0.0f, 0L),
            new SnippetArticle(KnownCategories.BOOKMARKS, "suggestion3", "Title3", "Publisher",
                    "preview", "junk1", getTimestamp(2017, Calendar.FEBRUARY, 20), 0.0f, 0L),
            new SnippetArticle(KnownCategories.BOOKMARKS, "suggestion4", "Title4", "Publisher",
                    "preview", "junk1", getTimestamp(2017, Calendar.MARCH, 30), 0.0f, 0L),
    };

    private static long getTimestamp(int year, int month, int day) {
        Calendar c = Calendar.getInstance();
        c.set(year, month, day);
        return c.getTimeInMillis();
    }

    @Override
    public void starting(Description description) {
        FakeSuggestionsSource fakeSuggestionsSource = new FakeSuggestionsSource();
        ContentSuggestionsTestUtils.registerCategory(
                fakeSuggestionsSource, KnownCategories.ARTICLES, 2);
        ContentSuggestionsTestUtils.registerCategory(
                fakeSuggestionsSource, KnownCategories.BOOKMARKS, 2);
        fakeSuggestionsSource.setSuggestionsForCategory(
                KnownCategories.ARTICLES, Arrays.asList(FAKE_ARTICLE_SUGGESTIONS));
        fakeSuggestionsSource.setSuggestionsForCategory(
                KnownCategories.BOOKMARKS, Arrays.asList(FAKE_BOOKMARK_SUGGESTIONS));
        final Bitmap favicon = BitmapFactory.decodeFile(
                UrlUtils.getTestFilePath("/android/UiCapture/favicon.ico"));
        Assert.assertNotNull(favicon);
        fakeSuggestionsSource.setFaviconForId("suggestion0", favicon);
        fakeSuggestionsSource.setThumbnailForId("suggestion0",
                BitmapFactory.decodeFile(UrlUtils.getTestFilePath("/android/google.png")));
        fakeSuggestionsSource.setThumbnailForId("suggestion1",
                BitmapFactory.decodeFile(
                        UrlUtils.getTestFilePath("/android/UiCapture/tower-bridge.jpg")));
        fakeSuggestionsSource.setThumbnailForId("suggestion2",
                BitmapFactory.decodeFile(
                        UrlUtils.getTestFilePath("/android/UiCapture/kettle.jpg")));
        fakeSuggestionsSource.setThumbnailForId("suggestion3",
                BitmapFactory.decodeFile(UrlUtils.getTestFilePath("/android/UiCapture/frog.jpg")));

        SuggestionsUiDelegateImpl.setSuggestionsSourceForTests(fakeSuggestionsSource);
        SuggestionsEventReporterBridge.setEventReporterForTests(
                new DummySuggestionsEventReporter());
        super.starting(description);
    }

    @Override
    protected void finished(Description description) {
        super.finished(description);
        SuggestionsEventReporterBridge.setEventReporterForTests(null);
        SuggestionsUiDelegateImpl.setSuggestionsSourceForTests(null);
    }
}
