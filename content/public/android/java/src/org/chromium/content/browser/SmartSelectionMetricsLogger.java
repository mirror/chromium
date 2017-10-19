// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.annotation.NonNull;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextSelection;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.content_public.browser.SelectionClient;
import org.chromium.content_public.browser.SelectionMetricsLogger;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.text.BreakIterator;
import java.util.regex.Pattern;

/**
 * Smart Selection logger, wrapper of Android logger methods.
 * We are logging word indices here. For one example:
 *     New York City , NY
 *    -1   0    1    2 3  4
 * We selected "York" at the first place, so York is [0, 1). Then we assume that Smart Selection
 * expanded the selection range to the whole "New York City , NY", we need to log [-1, 4). After
 * that, we single tap on "City", Smart Selection reset get triggered, we need to log [1, 2). Spaces
 * are ignored but we count each punctuation mark as a word.
 */
@TargetApi(Build.VERSION_CODES.O)
public class SmartSelectionMetricsLogger implements SelectionMetricsLogger {
    private static final String TAG = "SmartSelectionLogger";
    private static final Pattern PATTERN_WHITESPACE = Pattern.compile("[\\p{javaSpaceChar}\\s]+");

    private Context mContext;
    private Object mTracker;
    private Class<?> mTrackerClass;
    private Class<?> mSelectionEventClass;

    private boolean mEditable;

    private String mPreviousSelectionText;
    private int mPreviousSelectionStartOffset;
    private int mPreviousSelectionEndOffset;
    private int mOriginalSelectionStartOffset;
    private int[] mLastIndices;

    private final BreakIterator mWordIterator;

    public static SmartSelectionMetricsLogger create(WebContents webContents) {
        WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O || windowAndroid == null) return null;
        return new SmartSelectionMetricsLogger(windowAndroid.getContext().get());
    }

    @VisibleForTesting
    public static SmartSelectionMetricsLogger createForTesting(WebContents webContents) {
        WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        if (windowAndroid == null) return null;
        return new SmartSelectionMetricsLogger(windowAndroid.getContext().get());
    }

    private SmartSelectionMetricsLogger(Context context) {
        mContext = context;
        mTrackerClass = getTrackerClass();
        mSelectionEventClass = getSelectionEventClass();
        mWordIterator = BreakIterator.getWordInstance();
        mLastIndices = new int[2];
    }

    @VisibleForTesting
    protected SmartSelectionMetricsLogger() {
        mWordIterator = BreakIterator.getWordInstance();
        mLastIndices = new int[2];
    }

    @Override
    public void logSelectionStarted(String selectionText, int startOffset, boolean editable) {
        int endOffset = startOffset + selectionText.length();
        if (editable != mEditable || mTracker == null || mPreviousSelectionText == null
                || !overlap(mPreviousSelectionStartOffset, mPreviousSelectionEndOffset, startOffset,
                           endOffset)) {
            mTracker = createTracker(mContext, editable);
            mEditable = editable;
            updateSelectionState(selectionText, startOffset, /* force = */ true);
        } else {
            updateSelectionState(selectionText, startOffset, /* force = */ false);
        }

        mOriginalSelectionStartOffset = startOffset;

        if (!logConditionCheck()) {
            Log.d(TAG, "logConditionCheck failed.");
            return;
        }

        Log.d(TAG, "logSelectionStarted");
        logEvent(eventSelectionStarted(0));
    }

    @Override
    public void logSelectionModified(
            String selectionText, int startOffset, SelectionClient.Result result) {
        updateSelectionState(selectionText, startOffset, /* force = */ false);
        int endOffset = startOffset + selectionText.length();
        getWordDelta(startOffset, endOffset);
        if (!logConditionCheck()) {
            Log.d(TAG, "logConditionCheck failed.");
            return;
        }

        Log.d(TAG, "logSelectionModified");
        if (result != null && result.textSelection != null) {
            logEvent(
                    eventSelectionModified(mLastIndices[0], mLastIndices[1], result.textSelection));
        } else if (result != null && result.textClassification != null) {
            logEvent(eventSelectionModified(
                    mLastIndices[0], mLastIndices[1], result.textClassification));
        } else {
            logEvent(eventSelectionModified(mLastIndices[0], mLastIndices[1]));
        }
    }

    @Override
    public void logSelectionAction(String selectionText, int startOffset, @ActionType int action,
            SelectionClient.Result result) {
        int endOffset = startOffset + selectionText.length();
        getWordDelta(startOffset, endOffset);
        if (!logConditionCheck()) {
            Log.d(TAG, "logConditionCheck failed.");
            return;
        }

        Log.d(TAG, "logSelectionAction");
        if (result != null && result.textClassification != null) {
            logEvent(eventSelectionAction(
                    mLastIndices[0], mLastIndices[1], action, result.textClassification));
        } else {
            logEvent(eventSelectionAction(mLastIndices[0], mLastIndices[1], action));
        }
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

        // No overlap, update with the current selection.
        if (!overlap(mPreviousSelectionStartOffset, mPreviousSelectionEndOffset, startOffset,
                    endOffset)) {
            updateSelection(selectionText, startOffset);
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
    protected void getWordDelta(int start, int end) {
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

    private boolean logConditionCheck() {
        return mTracker != null && mSelectionEventClass != null;
    }

    private Object createTracker(Context context, boolean editable) {
        Object tracker = null;
        try {
            Constructor constructor =
                    mTrackerClass.getConstructor(new Class[] {Context.class, Integer.TYPE});
            tracker = constructor.newInstance(
                    context, editable ? /* EDIT_WEBVIEW */ 4 : /* WEBVIEW */ 2);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
        return tracker;
    }

    // Reflection wrapper of SmartSelectionEventTracker#logEvent(SelectionEvent)
    private void logEvent(Object selectionEvent) {
        if (!mSelectionEventClass.isInstance(selectionEvent)) {
            Log.w(TAG, "logEvent() only takes SelectionEvent type.");
            return;
        }

        try {
            Method logEventMethod =
                    mTrackerClass.getMethod("logEvent", new Class[] {mSelectionEventClass});
            logEventMethod.invoke(mTracker, mSelectionEventClass.cast(selectionEvent));
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
    }

    private Class<?> getTrackerClass() {
        String className = "android.view.textclassifier.logging.SmartSelectionEventTracker";
        try {
            Class<?> clazz = Class.forName(className);
            return clazz;
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    private Class<?> getSelectionEventClass() {
        try {
            return mTrackerClass.getClasses()[0];
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionStarted(int)
    private Object eventSelectionStarted(int start) {
        assert mSelectionEventClass != null;
        try {
            Method selectionStartedMethod =
                    mSelectionEventClass.getMethod("selectionStarted", new Class[] {Integer.TYPE});
            return selectionStartedMethod.invoke(null, start);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionModified(int, int)
    private Object eventSelectionModified(int start, int end) {
        assert mSelectionEventClass != null;
        try {
            Method selectionModifiedMethod = mSelectionEventClass.getMethod(
                    "selectionModified", new Class[] {Integer.TYPE, Integer.TYPE});
            return selectionModifiedMethod.invoke(null, start, end);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionModified(int, int, TextClassification)
    private Object eventSelectionModified(
            int start, int end, @NonNull TextClassification classification) {
        assert mSelectionEventClass != null;
        try {
            Method selectionModifiedMethod = mSelectionEventClass.getMethod("selectionModified",
                    new Class[] {Integer.TYPE, Integer.TYPE, TextClassification.class});
            return selectionModifiedMethod.invoke(null, start, end, classification);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionModified(int, int, TextSelection)
    private Object eventSelectionModified(int start, int end, @NonNull TextSelection selection) {
        assert mSelectionEventClass != null;
        try {
            Method selectionModifiedMethod = mSelectionEventClass.getMethod(
                    "selectionModified", new Class[] {Integer.TYPE, Integer.TYPE});
            return selectionModifiedMethod.invoke(null, start, end);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionAction(int, int, int)
    private Object eventSelectionAction(int start, int end, int actionType) {
        assert mSelectionEventClass != null;
        try {
            Method selectionActionMethod = mSelectionEventClass.getMethod(
                    "selectionAction", new Class[] {Integer.TYPE, Integer.TYPE, Integer.TYPE});
            return selectionActionMethod.invoke(null, start, end, actionType);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionAction(int, int, int, TextClassification)
    private Object eventSelectionAction(
            int start, int end, int actionType, @NonNull TextClassification classification) {
        assert mSelectionEventClass != null;
        try {
            Method selectionActionMethod = mSelectionEventClass.getMethod("selectionAction",
                    new Class[] {
                            Integer.TYPE, Integer.TYPE, Integer.TYPE, TextClassification.class});
            return selectionActionMethod.invoke(null, start, end, actionType, classification);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            Log.d(TAG, e.getMessage());
        }
        return null;
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
