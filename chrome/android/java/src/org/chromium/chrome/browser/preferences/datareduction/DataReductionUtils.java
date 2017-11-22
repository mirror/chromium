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
     * Sets TtsSpans of type TYPE_TEXT around "B" so that it is announced as "bytes" by
     * the TTS engine.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static CharSequence setSpanForBytes(String phrase) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            String[] data = phrase.split("\\s");
            if (data.length >= 2 && data[1].equals("B")) {
                Spannable phraseSpannable = new SpannableString(phrase);
                TtsSpan lSpanText = new TtsSpan.TextBuilder("bytes").build();
                phraseSpannable.setSpan(lSpanText, data[0].length(), phraseSpannable.length(), 0);
                return phraseSpannable;
            }
        }
        return phrase;
    }
}