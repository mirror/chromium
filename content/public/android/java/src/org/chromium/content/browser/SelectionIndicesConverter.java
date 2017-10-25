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

    private String mPreviousSelectionText;
    private int mPreviousSelectionStartOffset;
    private int mPreviousSelectionEndOffset;
    private int mOriginalSelectionStartOffset;
    private int[] mLastIndices;

    private final BreakIterator mWordIterator;

    SelectionIndicesConverter() {
        mWordIterator = BreakIterator.getWordInstance();
        mLastIndices = new int[2];
    }

    boolean needsUpdate(int startOffset, int endOffset) {
        if (mPreviousSelectionText == null) return true;
        return !overlap(
                mPreviousSelectionStartOffset, mPreviousSelectionEndOffset, startOffset, endOffset);
    }

    @VisibleForTesting
    public int[] getLastIndices() {
        return mLastIndices == null ? new int[2] : mLastIndices.clone();
    }

    @VisibleForTesting
    public String getSelectionText() {
        return mPreviousSelectionText;
    }

    @VisibleForTesting
    public int getSelectionStartOffset() {
        return mPreviousSelectionStartOffset;
    }

    @VisibleForTesting
    public int getOriginalSelectionStartOffset() {
        return mOriginalSelectionStartOffset;
    }

    @VisibleForTesting
    public void setSessionStartOffset(int startOffset) {
        mOriginalSelectionStartOffset = startOffset;
    }

    public void reset() {
        mPreviousSelectionText = null;
    }

    private void updateSelection(String selectionText, int startOffset) {
        mPreviousSelectionText = selectionText;
        mPreviousSelectionStartOffset = startOffset;
        mPreviousSelectionEndOffset = startOffset + selectionText.length();
    }

    @VisibleForTesting
    protected void updateSelectionState(String selectionText, int startOffset, boolean force) {
        int endOffset = startOffset + selectionText.length();
        if (force || mPreviousSelectionText == null) {
            updateSelection(selectionText, startOffset);
            return;
        }
        if (!overlap(mPreviousSelectionStartOffset, mPreviousSelectionEndOffset, startOffset,
                    endOffset)) {
            if (mPreviousSelectionEndOffset == startOffset) {
                // New selection is adjacent to the right of the last selection.
                updateSelection(
                        mPreviousSelectionText + selectionText, mPreviousSelectionStartOffset);
            } else if (mPreviousSelectionStartOffset == endOffset) {
                // New selection is adjacent on the left of the last selection.
                updateSelection(selectionText + mPreviousSelectionText, startOffset);
            } else {
                // No overlap, update with the current selection.
                updateSelection(selectionText, startOffset);
            }
            return;
        }

        // Previous selection contains current selection, no need to do anything.
        if (contains(mPreviousSelectionStartOffset, mPreviousSelectionEndOffset, startOffset,
                    endOffset)) {
            return;
        }

        // Current selection contains previous selection, update with current selection.
        if (contains(startOffset, endOffset, mPreviousSelectionStartOffset,
                    mPreviousSelectionEndOffset)) {
            updateSelection(selectionText, startOffset);
            return;
        }

        // Handle concatenation cases.
        if (startOffset < mPreviousSelectionStartOffset) {
            updateSelection(selectionText.substring(0, mPreviousSelectionStartOffset - startOffset)
                            + mPreviousSelectionText,
                    startOffset);
        } else {
            updateSelection(
                    mPreviousSelectionText.substring(0, startOffset - mPreviousSelectionStartOffset)
                            + selectionText,
                    mPreviousSelectionStartOffset);
        }
    }

    @VisibleForTesting
    protected void setOriginalOffset(int offset) {
        mOriginalSelectionStartOffset = offset;
    }

    @VisibleForTesting
    protected int[] getWordDelta(int start, int end) {
        mWordIterator.setText(mPreviousSelectionText);
        start = start - mPreviousSelectionStartOffset;
        end = end - mPreviousSelectionStartOffset;
        int originalOffset = mOriginalSelectionStartOffset - mPreviousSelectionStartOffset;

        if (start == originalOffset) {
            mLastIndices[0] = 0;
        } else if (start < originalOffset) {
            mLastIndices[0] = -countWordsForward(start, originalOffset);
        } else {
            // start > originalOffset
            mLastIndices[0] = countWordsBackward(start, originalOffset);
            // For the selection start index, avoid counting a partial word backwards.
            if (!mWordIterator.isBoundary(start)
                    && !isWhitespace(
                               mWordIterator.preceding(start), mWordIterator.following(start))) {
                // We counted a partial word. Remove it.
                mLastIndices[0]--;
            }
        }

        if (end == originalOffset) {
            mLastIndices[1] = 0;
        } else if (end < originalOffset) {
            mLastIndices[1] = -countWordsForward(end, originalOffset);
        } else {
            // end > originalOffset
            mLastIndices[1] = countWordsBackward(end, originalOffset);
        }

        return mLastIndices;
    }

    @VisibleForTesting
    protected int countWordsBackward(int from, int originalOffset) {
        assert from >= originalOffset;
        int wordCount = 0;
        int offset = from;
        while (offset > originalOffset) {
            int start = mWordIterator.preceding(offset);
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
            int end = mWordIterator.following(offset);
            if (!isWhitespace(offset, end)) {
                wordCount++;
            }
            offset = end;
        }
        return wordCount;
    }

    @VisibleForTesting
    protected boolean isWhitespace(int start, int end) {
        return PATTERN_WHITESPACE.matcher(mPreviousSelectionText.substring(start, end)).matches();
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
