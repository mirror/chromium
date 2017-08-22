// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

/**
 * Represents an entry in a text suggestion popup menu. Contains the information
 * necessary to display the menu entry and the information necessary to apply
 * the suggestion.
 */
public class SuggestionInfo {
    private final int mMarkerTag;
    private final int mSuggestionIndex;
    private final String mPrefix;
    private final String mSuggestion;
    private final String mSuffix;

    SuggestionInfo(
            int markerTag, int suggestionIndex, String prefix, String suggestion, String suffix) {
        mMarkerTag = markerTag;
        mSuggestionIndex = suggestionIndex;
        mPrefix = prefix;
        mSuggestion = suggestion;
        mSuffix = suffix;
    }

    public int getMarkerTag() {
        return mMarkerTag;
    }

    public int getSuggestionIndex() {
        return mSuggestionIndex;
    }

    public String getPrefix() {
        return mPrefix;
    }

    public String getSuggestion() {
        return mSuggestion;
    }

    public String getSuffix() {
        return mSuffix;
    }
}
