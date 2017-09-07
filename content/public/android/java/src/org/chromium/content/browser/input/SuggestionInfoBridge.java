// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.input;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Helper class for creating SuggestionInfo objects from C++.
 */
@JNINamespace("content")
public class SuggestionInfoBridge {
    private SuggestionInfoBridge() {}

    /**
     * Creates an array to hold SuggestionInfos.
     * @param length The number of elements the newly-created array will have.
     */
    @CalledByNative
    private static SuggestionInfo[] createArray(int length) {
        return new SuggestionInfo[length];
    }

    /**
     * Creates a new SuggestionInfo object and sets it as an element in the passed-in array.
     * @param suggestionInfos The array to put the newly-created SuggestionInfo in.
     * @param index The index in suggestionInfos to put the newly-created SuggestionInfo in.
     * @param markerTag Used as an opaque identifier to tell Blink which suggestion was picked.
     * @param suggestionIndex Used as an opaque identifier to tell Blink which suggestion was
     * picked.
     * @param prefix Text at the beginning of the highlighted suggestion region that will not be
     * changed by applying the suggestion.
     * @param suggestion Text that will replace the text between the prefix and suffix strings if
     * the suggestion is applied.
     * @param suffix Text at the end of the highlighted suggestion region that will not be
     * changed by applying the suggestion.
     * @param suffix Text to show in the suggestion menu grayed-out before the actual suggestion.
     */
    @CalledByNative
    private static void createSuggestionInfoAndPutInArray(SuggestionInfo[] suggestionInfos,
            int index, int markerTag, int suggestionIndex, String prefix, String suggestion,
            String suffix) {
        SuggestionInfo suggestionInfo =
                new SuggestionInfo(markerTag, suggestionIndex, prefix, suggestion, suffix);
        suggestionInfos[index] = suggestionInfo;
    }
}
