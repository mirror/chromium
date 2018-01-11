// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.signin.util;

import java.util.ArrayList;
import java.util.List;

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

    private final List<String> mChunks = new ArrayList<>();

    /**
     * Constructs PatternMatcher and pre-processes pattern for faster matching.
     *
     * @param pattern the pattern to match strings against. Please see JavaDoc for class for
     *         the format description.
     */
    public PatternMatcher(String pattern) throws IllegalPatternException {
        // Split pattern by * wildcards and un-escape.
        boolean escape = false;
        StringBuilder currentChunk = new StringBuilder();
        for (int i = 0; i < pattern.length(); i++) {
            char patternChar = pattern.charAt(i);
            if (!escape && patternChar == '\\') {
                escape = true;
                continue;
            }
            if (!escape && patternChar == '*') {
                mChunks.add(currentChunk.toString());
                currentChunk = new StringBuilder();
                continue;
            }
            currentChunk.append(patternChar);
            escape = false;
        }
        if (escape) {
            throw new IllegalPatternException("Unmatched escape symbol at the end", pattern);
        }
        mChunks.add(currentChunk.toString());
    }

    /**
     * Checks whether the given string matches the pattern.
     *
     * @param string the string to match against the pattern.
     * @return whether the whole string matches the pattern.
     */
    public boolean isMatch(String string) {
        // No wildcards, the whole string should match the pattern.
        if (mChunks.size() == 1) {
            return string.equals(mChunks.get(0));
        }
        // The first chunk should match the string head.
        String firstChunk = mChunks.get(0);
        if (!string.startsWith(firstChunk)) return false;
        // The last chunk should match the string tail.
        String lastChunk = mChunks.get(mChunks.size() - 1);
        if (!string.endsWith(lastChunk)) return false;
        // Greedy match all the rest of the chunks.
        int stringOffset = firstChunk.length();
        for (int i = 1; i < mChunks.size() - 1; i++) {
            int offset = string.indexOf(mChunks.get(i), stringOffset);
            if (offset == -1) return false;
            stringOffset = offset + mChunks.get(i).length();
        }
        return stringOffset + lastChunk.length() <= string.length();
    }
}
