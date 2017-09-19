// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.snippets;

import java.util.List;

/**
 * Callbacks for the {@link SuggestionsSource#fetchSuggestions} method. Only one of these methods
 * will be called per {@link SuggestionsSource#fetchSuggestions} call.
 */
public interface FetchSuggestionsCallbacks {
    /** Called when more suggestions are available. */
    void onMoreSuggestions(List<SnippetArticle> suggestions);

    /** Called on failure. */
    void onFailure();
}
