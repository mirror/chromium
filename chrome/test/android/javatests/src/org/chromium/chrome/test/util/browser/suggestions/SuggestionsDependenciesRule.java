// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser.suggestions;

import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

import org.chromium.chrome.browser.favicon.LargeIconBridge;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.MostVisitedSites;
import org.chromium.chrome.browser.suggestions.SuggestionsDependencyFactory;
import org.chromium.chrome.browser.suggestions.SuggestionsEventReporter;

/**
 * Rule that allows mocking native dependencies of the suggestions package.
 *
 * The Factory members to override should be set before the main test rule is called to initialise
 * the test activity.
 *
 * @see SuggestionsDependencyFactory
 */
public class SuggestionsDependenciesRule extends TestWatcher {
    private TestFactory mFactory;

    public TestFactory getFactory() {
        return mFactory;
    }

    @Override
    protected void starting(Description description) {
        mFactory = new TestFactory();
        SuggestionsDependencyFactory.setInstanceForTesting(mFactory);
    }

    @Override
    protected void finished(Description description) {
        SuggestionsDependencyFactory.setInstanceForTesting(null);
    }

    /**
     * SuggestionsDependencyFactory that exposes and allows modifying the instances to be injected.
     */
    public static class TestFactory extends SuggestionsDependencyFactory {
        public SuggestionsSource mSuggestionsSource;
        public MostVisitedSites mMostVisitedSites;
        public LargeIconBridge mLargeIconBridge;
        public SuggestionsEventReporter mEventReporter;

        @Override
        public SuggestionsSource createSuggestionSource(Profile profile) {
            if (mSuggestionsSource != null) return mSuggestionsSource;
            return super.createSuggestionSource(profile);
        }

        @Override
        public SuggestionsEventReporter createEventReporter() {
            if (mEventReporter != null) return mEventReporter;
            return super.createEventReporter();
        }

        @Override
        public MostVisitedSites createMostVisitedSites(Profile profile) {
            if (mMostVisitedSites != null) return mMostVisitedSites;
            return super.createMostVisitedSites(profile);
        }

        @Override
        public LargeIconBridge createLargeIconBridge(Profile profile) {
            if (mLargeIconBridge != null) return mLargeIconBridge;
            return new LargeIconBridge(profile);
        }
    }
}
