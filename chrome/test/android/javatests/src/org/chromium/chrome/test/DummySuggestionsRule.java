// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test;

import static org.chromium.chrome.test.util.browser.suggestions.ContentSuggestionsTestUtils.registerCategory;

import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

import org.chromium.chrome.browser.suggestions.SuggestionsEventReporterBridge;
import org.chromium.chrome.browser.suggestions.SuggestionsUiDelegateImpl;
import org.chromium.chrome.test.util.browser.suggestions.DummySuggestionsEventReporter;
import org.chromium.chrome.test.util.browser.suggestions.FakeSuggestionsSource;

/**
 * A canned, inflexible, set of pretty suggestions for use in Ui cataloging tests.
 */
public class DummySuggestionsRule extends TestWatcher {
    @Override
    public void starting(Description description) {
        FakeSuggestionsSource suggestionsSource = new FakeSuggestionsSource();
        registerCategory(suggestionsSource, /* category = */ 42, /* count = */ 5);

        SuggestionsUiDelegateImpl.setSuggestionsSourceForTests(suggestionsSource);
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
