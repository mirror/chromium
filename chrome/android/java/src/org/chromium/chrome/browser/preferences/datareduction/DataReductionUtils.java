// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.datareduction;

import android.annotation.TargetApi;
import android.os.Build;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.style.TtsSpan;

/**
 * Helper functions for data saver info that is shown to the user.
 */
public class DataReductionUtils {
    /**
     * Sets TtsSpans of type TYPE_MEASURE so that "B" is announced as "byte" by
     * the TTS engine.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static CharSequence setSpanForBytes(long number, String phrase) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            // Only add spans for numbers which will be displayed as bytes.
            if (number / 1000 < 1) {
                TtsSpan ttsSpan =
                        new TtsSpan.MeasureBuilder().setNumber(number).setUnit("byte").build();
                Spannable phraseSpannable = new SpannableString(phrase);
                phraseSpannable.setSpan(ttsSpan, 0, phraseSpannable.length(), 0);
                return phraseSpannable;
            }
        }
        return phrase;
    }
}