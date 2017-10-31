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
    private int mGlobalEndOffset;

    // Tracking previous selection.
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

    @VisibleForTesting
    public String getGlobalSelectionText() {
        return mGlobalSelectionText;
    }

    @VisibleForTesting
    public int getGlobalStartOffset() {
        return mGlobalStartOffset;
    }

    public void reset() {
        mGlobalSelectionText = null;
        mGlobalStartOffset = -1;
        mGlobalEndOffset = -1;
        mLastSelectionText = null;
        mLastStartOffset = -1;
        mInitialStartOffset = -1;
    }

    private void updateLastSelection(String selectionText, int startOffset) {
        mLastSelectionText = selectionText;
        mLastStartOffset = startOffset;
        mLastEndOffset = startOffset + selectionText.length();
    }

    private void updateGlobalSelection(String selectionText, int startOffset) {
        mGlobalSelectionText = selectionText;
        mGlobalStartOffset = startOffset;
        mGlobalEndOffset = startOffset + selectionText.length();
    }

    private void combineGlobalSelection(String selectionText, int startOffset) {
        int endOffset = startOffset + selectionText.length();
        if (!overlap(mGlobalStartOffset, mGlobalEndOffset, startOffset, endOffset)) {
            if (mGlobalEndOffset == startOffset) {
                // New selection is adjacent to the right of the global selection.
                updateGlobalSelection(mGlobalSelectionText + selectionText, mGlobalStartOffset);
            } else if (mGlobalStartOffset == endOffset) {
                // New selection is adjacent on the left of the global selection.
                updateGlobalSelection(selectionText + mGlobalSelectionText, startOffset);
            } else {
                // No overlap, update with the current selection.
                updateGlobalSelection(selectionText, startOffset);
            }
            return;
        }

        // Previous selection contains current selection, no need to do anything.
        if (contains(mGlobalStartOffset, mGlobalEndOffset, startOffset, endOffset)) {
            return;
        }

        // Current selection contains previous selection, update with current selection.
        if (contains(startOffset, endOffset, mGlobalStartOffset, mGlobalEndOffset)) {
            updateGlobalSelection(selectionText, startOffset);
            return;
        }

        // Handle concatenation cases.
        if (startOffset < mGlobalStartOffset) {
            updateGlobalSelection(selectionText.substring(0, mGlobalStartOffset - startOffset)
                            + mGlobalSelectionText,
                    startOffset);
        } else {
            updateGlobalSelection(
                    mGlobalSelectionText.substring(0, startOffset - mGlobalStartOffset)
                            + selectionText,
                    mGlobalStartOffset);
        }
    }

    @VisibleForTesting
    protected boolean updateSelectionState(String selectionText, int startOffset, boolean force) {
        int endOffset = startOffset + selectionText.length();
        if (force || mGlobalSelectionText == null) {
            updateLastSelection(selectionText, startOffset);
            updateGlobalSelection(selectionText, startOffset);
            return true;
        }

        boolean update = false;
        if (overlap(mLastStartOffset, mLastEndOffset, startOffset, endOffset)) {
            int l = Math.max(mLastStartOffset, startOffset);
            int r = Math.min(mLastEndOffset, endOffset);
            boolean same = true;
            for (int i = l; i < r; ++i) {
                if (mLastSelectionText.charAt(i - mLastStartOffset)
                        != selectionText.charAt(i - startOffset)) {
                    same = false;
                    break;
                }
            }
            update = same;
        } else if (mLastStartOffset == endOffset || mLastEndOffset == startOffset) {
            update = true;
        }

        if (!update) {
            reset();
            return false;
        }

        updateLastSelection(selectionText, startOffset);
        combineGlobalSelection(selectionText, startOffset);
        return true;
    }

    @VisibleForTesting
    protected void setInitialStartOffset(int offset) {
        mInitialStartOffset = offset;
    }

    @VisibleForTesting
    protected void setIteratorText(String text) {
        mBreakIterator.setText(text);
    }

    @VisibleForTesting
    protected boolean getWordDelta(int start, int end, int[] wordIndices) {
        assert wordIndices.length == 2;
        wordIndices[0] = wordIndices[1] = 0;

        start = start - mGlobalStartOffset;
        end = end - mGlobalStartOffset;
        if (start >= end) return false;
        if (start < 0 || start >= mGlobalSelectionText.length()) return false;
        if (end <= 0 || end > mGlobalSelectionText.length()) return false;

        int originalOffset = mInitialStartOffset - mGlobalStartOffset;
        if (originalOffset < 0 && originalOffset >= mGlobalSelectionText.length()) return false;

        mBreakIterator.setText(mGlobalSelectionText);
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

        return true;
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
        return PATTERN_WHITESPACE.matcher(mGlobalSelectionText.substring(start, end)).matches();
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
