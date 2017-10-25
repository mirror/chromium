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

    // Tracking the selection context.
    private String mLastSelectionText;
    private int mLastStartOffset;
    private int mLastEndOffset;

    // The start offset from SelectionStarted call.
    private int mInitialStartOffset;

    // A WordInstance of BreakIterator.
    private final BreakIterator mBreakIterator;

    SelectionIndicesConverter() {
        mBreakIterator = BreakIterator.getWordInstance();
    }

    boolean needsUpdate(int startOffset, int endOffset) {
        if (mLastSelectionText == null) return true;
        return !overlap(mLastStartOffset, mLastEndOffset, startOffset, endOffset);
    }

    @VisibleForTesting
    public String getSelectionText() {
        return mLastSelectionText;
    }

    @VisibleForTesting
    public int getSelectionStartOffset() {
        return mLastStartOffset;
    }

    @VisibleForTesting
    public int getOriginalSelectionStartOffset() {
        return mInitialStartOffset;
    }

    public void reset() {
        mLastSelectionText = null;
    }

    private void updateSelection(String selectionText, int startOffset) {
        mLastSelectionText = selectionText;
        mLastStartOffset = startOffset;
        mLastEndOffset = startOffset + selectionText.length();
    }

    @VisibleForTesting
    protected void updateSelectionState(String selectionText, int startOffset, boolean force) {
        int endOffset = startOffset + selectionText.length();
        if (force || mLastSelectionText == null) {
            updateSelection(selectionText, startOffset);
            return;
        }
        if (!overlap(mLastStartOffset, mLastEndOffset, startOffset, endOffset)) {
            if (mLastEndOffset == startOffset) {
                // New selection is adjacent to the right of the last selection.
                updateSelection(mLastSelectionText + selectionText, mLastStartOffset);
            } else if (mLastStartOffset == endOffset) {
                // New selection is adjacent on the left of the last selection.
                updateSelection(selectionText + mLastSelectionText, startOffset);
            } else {
                // No overlap, update with the current selection.
                updateSelection(selectionText, startOffset);
            }
            return;
        }

        // Previous selection contains current selection, no need to do anything.
        if (contains(mLastStartOffset, mLastEndOffset, startOffset, endOffset)) {
            return;
        }

        // Current selection contains previous selection, update with current selection.
        if (contains(startOffset, endOffset, mLastStartOffset, mLastEndOffset)) {
            updateSelection(selectionText, startOffset);
            return;
        }

        // Handle concatenation cases.
        if (startOffset < mLastStartOffset) {
            updateSelection(
                    selectionText.substring(0, mLastStartOffset - startOffset) + mLastSelectionText,
                    startOffset);
        } else {
            updateSelection(
                    mLastSelectionText.substring(0, startOffset - mLastStartOffset) + selectionText,
                    mLastStartOffset);
        }
    }

    @VisibleForTesting
    protected void setInitialStartOffset(int offset) {
        mInitialStartOffset = offset;
    }

    @VisibleForTesting
    protected int[] getWordDelta(int start, int end) {
        assert start <= end;
        mBreakIterator.setText(mLastSelectionText);
        start = start - mLastStartOffset;
        end = end - mLastStartOffset;
        assert 0 <= start && start < mLastSelectionText.length();
        assert 0 <= end && end <= mLastSelectionText.length();

        int originalOffset = mInitialStartOffset - mLastStartOffset;
        assert 0 <= originalOffset && originalOffset < mLastSelectionText.length();

        int[] wordIndices = new int[2];
        if (start <= originalOffset) {
            wordIndices[0] = -countWordsForward(start, originalOffset);
        } else {
            // start > originalOffset
            wordIndices[0] = countWordsBackward(start, originalOffset);
            // For the selection start index, avoid counting a partial word backwards.
            if (!mBreakIterator.isBoundary(start)
                    && !isWhitespace(
                               mBreakIterator.preceding(start), mBreakIterator.following(start))) {
                // We counted a partial word. Remove it.
                wordIndices[0]--;
            }
        }

        if (end <= originalOffset) {
            wordIndices[1] = -countWordsForward(end, originalOffset);
        } else {
            // end > originalOffset
            wordIndices[1] = countWordsBackward(end, originalOffset);
        }

        return wordIndices;
    }

    @VisibleForTesting
    protected int countWordsBackward(int from, int originalOffset) {
        assert from >= originalOffset;
        int wordCount = 0;
        int offset = from;
        while (offset > originalOffset) {
            int start = mBreakIterator.preceding(offset);
            if (!isWhitespace(start, offset)) {
                wordCount++;
            }
            offset = start;
        }
        return wordCount;
    }

    @VisibleForTesting
    protected int countWordsForward(int from, int originalOffset) {
        assert from <= originalOffset;
        int wordCount = 0;
        int offset = from;
        while (offset < originalOffset) {
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
        return PATTERN_WHITESPACE.matcher(mLastSelectionText.substring(start, end)).matches();
    }

    // Check if [al, ar) overlaps [bl, br).
    public static boolean overlap(int al, int ar, int bl, int br) {
        if (al <= bl) {
            return bl < ar;
        }
        return br > al;
    }

    // Check if [al, ar) contains [bl, br).
    public static boolean contains(int al, int ar, int bl, int br) {
        return al <= bl && br <= ar;
    }
}
