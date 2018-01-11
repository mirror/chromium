// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.signin.util;

import java.util.ArrayList;

/**
 * A simple class that encapsulates simple wildcard pattern-matching.
 *
 * Always tries to match the whole string to the pattern. Special characters for patterns are:
 * Asterisk * - matches any sequence of characters (including empty sequence);
 * Backslash \ - escapes the following character, so \* will match literal asterisk only. May also
 *     escape characters that don't have any special meaning, so 't' and '\t' are the same patterns.
 *
 * See unit tests for usage examples.
 */
public class PatternMatcher {
    /** Encapsulates information about illegal pattern. */
    public static class IllegalPatternException extends Exception {
        private IllegalPatternException(String message, String pattern) {
            super(String.format("Illegal pattern '%s': %s", pattern, message));
        }
    }

    private final String[] mChunks;

    /**
     * Constructs PatternMatcher and pre-processes pattern for faster matching.
     *
     * @param pattern the pattern to match strings against. Please see JavaDoc for class for
     *         the format description.
     */
    public PatternMatcher(String pattern) throws IllegalPatternException {
        // Split pattern by * wildcards and un-escape.
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
            throw new IllegalPatternException("Unmatched escape symbol at the end", pattern);
        }
        chunks.add(currentChunk.toString());
        mChunks = chunks.toArray(new String[0]);
    }

    /**
     * Checks whether the given string matches the pattern.
     *
     * @param string the string to match against the pattern.
     * @return whether the whole string matches the pattern.
     */
    public boolean isMatch(String string) {
        // No wildcards, the whole string should match the pattern.
        if (mChunks.length == 1) {
            return string.equals(mChunks[0]);
        }
        // Check that last chunk matches the string tail.
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
