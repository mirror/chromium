// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.signin.util;

import java.util.ArrayList;

/**
 * A simple class that encapsulates simple wildcard pattern-matching. Tries to match the whole
 * string to the pattern. The only characters with special treatment are:
 * Asterisk * - matches any sequence of characters (including empty sequence);
 * Backslash \ - escapes the following character, so \* will match literal asterisk only. May also
 *     escape characters that don't have any special meaning, so 't' and '\t' are the same patterns.
 * See {@link PatternMatcherTest} for usage examples.
 */
public class PatternMatcher {
    private static final String TAG = "PatternMatcher";

    private final String[] mChunks;

    public PatternMatcher(String pattern) {
        // Split pattern by * wildcards and un-escaped.
        ArrayList<String> chunks = new ArrayList<>();
        boolean escape = false;
        StringBuilder currentChunk = new StringBuilder();
        for (int i = 0; i < pattern.length(); i++) {
            char patternChar = pattern.charAt(i);
            if (!escape && patternChar == '\\') {
                escape = true;
                continue;
            }
            if (!escape && patternChar == '*') {
                chunks.add(currentChunk.toString());
                currentChunk = new StringBuilder();
                continue;
            }
            currentChunk.append(patternChar);
            escape = false;
        }
        if (escape) {
            throw new IllegalArgumentException(String.format("Malformed pattern: '%s'", pattern));
        }
        chunks.add(currentChunk.toString());
        mChunks = chunks.toArray(new String[0]);
    }

    public boolean isMatch(String string) {
        // No wildcards, should
        if (mChunks.length == 1) {
            return string.equals(mChunks[0]);
        }
        // Check that last chunk is present in the input string
        String lastChunk = mChunks[mChunks.length - 1];
        if (!string.endsWith(lastChunk)) return false;

        // Greedy match all the chunks except the last one.
        int stringOffset = 0;
        for (int i = 0; i < mChunks.length - 1; i++) {
            int offset = string.indexOf(mChunks[i], stringOffset);
            if (offset == -1) return false;
            stringOffset += offset + mChunks[i].length();
        }
        return stringOffset + lastChunk.length() <= string.length();
    }
}
