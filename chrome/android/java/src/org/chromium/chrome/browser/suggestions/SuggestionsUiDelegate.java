// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import org.chromium.base.DiscardableReferencePool;
import org.chromium.chrome.browser.ntp.snippets.SuggestionsSource;
import org.chromium.content_public.browser.WebContents;

import javax.annotation.Nullable;

/**
 * Interface between the suggestion surface and the rest of the browser.
 */
public interface SuggestionsUiDelegate {
    // Dependency injection
    // TODO(dgn): remove these methods once the users have a different way to get a reference
    // to these objects (https://crbug.com/677672)

    /** Convenience method to access the {@link SuggestionsSource}. */
    SuggestionsSource getSuggestionsSource();

    /** Convenience method to access the {@link SuggestionsRanker}. */
    SuggestionsRanker getSuggestionsRanker();

    /** Convenience method to access the {@link SuggestionsEventReporter}. */
    SuggestionsEventReporter getEventReporter();

    /** Convenience method to access the {@link SuggestionsNavigationDelegate}. */
    SuggestionsNavigationDelegate getNavigationDelegate();

    /** Convenience method to access the {@link WebContents} of the current active tab associated to
     * this delegate's host, if there is one. */
    @Nullable
    WebContents getWebContents();

    /** Convenience method to access the {@link ImageFetcher} */
    ImageFetcher getImageFetcher();

    /**
     * @return The reference pool to use for large objects that should be dropped under
     * memory pressure.
     */
    DiscardableReferencePool getReferencePool();

    // Feature/State checks

    /**
     * Registers a {@link DestructionObserver}, notified when the New Tab Page goes away.
     */
    void addDestructionObserver(DestructionObserver destructionObserver);

    /** @return Whether the suggestions UI is currently visible. */
    boolean isVisible();
}