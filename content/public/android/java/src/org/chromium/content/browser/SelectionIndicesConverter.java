// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import org.chromium.base.VisibleForTesting;

import java.text.BreakIterator;
import java.util.regex.Pattern;

/**
 * Converts character indices to relative word indices.
 */
public class SelectionIndicesConverter {
    private static final Pattern PATTERN_WHITESPACE = Pattern.compile("[\\p{javaSpaceChar}\\s]+");

    // Tracking the overall selection during current logging session.
    private String mGlobalSelectionText;
    private int mGlobalStartOffset;

    // Tracking previous selection.
    private String mLastSelectionText;
    private int mLastStartOffset;

    // The start offset from SelectionStarted call.
    private int mInitialStartOffset;

    // A WordInstance of BreakIterator.
    private final BreakIterator mBreakIterator;

    SelectionIndicesConverter() {
        mBreakIterator = BreakIterator.getWordInstance();
    }

    // Try to update global selection with the current selection so we could use global selection
    // information to calculate relative word indices. Due to the DOM tree change, startOffset is
    // not very reliable. We need to invalidate loging session if we detected such changes.
    public boolean updateSelectionState(String selectionText, int startOffset) {
        if (mGlobalSelectionText == null) {
            updateLastSelection(selectionText, startOffset);
            updateGlobalSelection(selectionText, startOffset);
            return true;
        }

        boolean update = false;
        int endOffset = startOffset + selectionText.length();
        int lastEndOffset = mLastStartOffset + mLastSelectionText.length();
        if (overlap(mLastStartOffset, lastEndOffset, startOffset, endOffset)) {
            // We need to compare the overlapping part to make sure that we can update it.
            int l = Math.max(mLastStartOffset, startOffset);
            int r = Math.min(lastEndOffset, endOffset);
            update = mLastSelectionText.regionMatches(
                    l - mLastStartOffset, selectionText, l - startOffset, r - l);
        } else if (mLastStartOffset == endOffset || lastEndOffset == startOffset) {
            // Handle adjacent cases.
            update = true;
        }
        if (!update) {
            mGlobalSelectionText = null;
            mLastSelectionText = null;
            return false;
        }

        updateLastSelection(selectionText, startOffset);
        combineGlobalSelection(selectionText, startOffset);
        return true;
    }

    public boolean getWordDelta(int start, int end, int[] wordIndices) {
        assert wordIndices.length == 2;
        wordIndices[0] = wordIndices[1] = 0;

        start = start - mGlobalStartOffset;
        end = end - mGlobalStartOffset;
        if (start >= end) return false;
        if (start < 0 || start >= mGlobalSelectionText.length()) return false;
        if (end <= 0 || end > mGlobalSelectionText.length()) return false;

        int initialStartOffset = mInitialStartOffset - mGlobalStartOffset;
        mBreakIterator.setText(mGlobalSelectionText);

        if (start <= initialStartOffset) {
            wordIndices[0] = -countWordsForward(start, initialStartOffset);
        } else {
            // start > initialStartOffset
            wordIndices[0] = countWordsBackward(start, initialStartOffset);
            // For the selection start index, avoid counting a partial word backwards.
            if (!mBreakIterator.isBoundary(start)
                    && !isWhitespace(
                               mBreakIterator.preceding(start), mBreakIterator.following(start))) {
                // We counted a partial word. Remove it.
                wordIndices[0]--;
            }
        }

        if (end <= initialStartOffset) {
            wordIndices[1] = -countWordsForward(end, initialStartOffset);
        } else {
            // end > initialStartOffset
            wordIndices[1] = countWordsBackward(end, initialStartOffset);
        }

        return true;
    }

    public void setInitialStartOffset(int offset) {
        mInitialStartOffset = offset;
    }

    @VisibleForTesting
    protected String getGlobalSelectionText() {
        return mGlobalSelectionText;
    }

    @VisibleForTesting
    protected int getGlobalStartOffset() {
        return mGlobalStartOffset;
    }

    @VisibleForTesting
    protected void setIteratorText(String text) {
        mBreakIterator.setText(text);
    }

    // Count how many words from "from" to "initialStartOffset", "from" should greater than or equal
    // to "initialStartOffset", punctuations are counted as words. Duplicated from Android.
    @VisibleForTesting
    protected int countWordsBackward(int from, int initialStartOffset) {
        assert from >= initialStartOffset;
        int wordCount = 0;
        int offset = from;
        while (offset > initialStartOffset) {
            int start = mBreakIterator.preceding(offset);
            if (!isWhitespace(start, offset)) {
                wordCount++;
            }
            offset = start;
        }
        return wordCount;
    }

    // Count how many words from "from" to "initialStartOffset", "from" should less than or equal
    // to "initialStartOffset", punctuations are counted as words. Duplicated from Android.
    @VisibleForTesting
    protected int countWordsForward(int from, int initialStartOffset) {
        assert from <= initialStartOffset;
        int wordCount = 0;
        int offset = from;
        while (offset < initialStartOffset) {
            int end = mBreakIterator.following(offset);
            if (!isWhitespace(offset, end)) {
                wordCount++;
            }
            offset = end;
        }
        return wordCount;
    }

    @VisibleForTesting
    protected boolean isWhitespace(int start, int end) {
        return PATTERN_WHITESPACE.matcher(mGlobalSelectionText.substring(start, end)).matches();
    }

    // Check if [al, ar) overlaps [bl, br).
    @VisibleForTesting
    protected static boolean overlap(int al, int ar, int bl, int br) {
        if (al <= bl) {
            return bl < ar;
        }
        return br > al;
    }

    // Check if [al, ar) contains [bl, br).
    @VisibleForTesting
    protected static boolean contains(int al, int ar, int bl, int br) {
        return al <= bl && br <= ar;
    }

    private void updateLastSelection(String selectionText, int startOffset) {
        mLastSelectionText = selectionText;
        mLastStartOffset = startOffset;
    }

    private void updateGlobalSelection(String selectionText, int startOffset) {
        mGlobalSelectionText = selectionText;
        mGlobalStartOffset = startOffset;
    }

    // Use current selection to gradually update global selection.
    // Within each selection logging session, we obtain the next selection from shrink, expand or
    // reverse select the current selection. To update global selection, we only need to extend both
    // sides of the last global selection with current selection if necessary.
    private void combineGlobalSelection(String selectionText, int startOffset) {
        int endOffset = startOffset + selectionText.length();
        int globalEndOffset = mGlobalStartOffset + mGlobalSelectionText.length();

        // Extends left if necessary.
        if (startOffset < mGlobalStartOffset) {
            updateGlobalSelection(selectionText.substring(0, mGlobalStartOffset - startOffset)
                            + mGlobalSelectionText,
                    startOffset);
        }

        // Extends right if necessary.
        if (endOffset > globalEndOffset) {
            updateGlobalSelection(mGlobalSelectionText
                            + selectionText.substring(
                                      globalEndOffset - startOffset, endOffset - startOffset),
                    mGlobalStartOffset);
        }
    }
}
