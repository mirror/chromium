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
import org.chromium.content_public.browser.SelectionClient;
import org.chromium.content_public.browser.SelectionMetricsLogger;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

/**
 * Smart Selection logger, wrapper of Android logger methods.
 */
@TargetApi(Build.VERSION_CODES.O)
public class SmartSelectionMetricsLogger implements SelectionMetricsLogger {
    private static final String TAG = "SmartSelectionLogger";

    private Context mContext;
    private Object mTracker;
    private Class<?> mTrackerClass;
    private Class<?> mSelectionEventClass;

    private boolean mEditable;

    private String mPreviousSelectionText;
    private int mPreviousSelectionStartOffset;
    private int mPreviousSelectionEndOffset;

    public static SmartSelectionMetricsLogger create(WebContents webContents) {
        WindowAndroid windowAndroid = webContents.getTopLevelNativeWindow();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O || windowAndroid == null) return null;
        return new SmartSelectionMetricsLogger(windowAndroid.getContext().get());
    }

    private SmartSelectionMetricsLogger(Context context) {
        mContext = context;
        mTrackerClass = getTrackerClass();
        mSelectionEventClass = getSelectionEventClass();
    }

    @Override
    public void logSelectionStarted(String selectionText, int startOffset, boolean editable) {
        int endOffset = startOffset + selectionText.length();
        if (editable != mEditable || mTracker == null || mPreviousSelectionText == null
                || !overlap(mPreviousSelectionStartOffset, mPreviousSelectionEndOffset, startOffset,
                           endOffset)) {
            mTracker = createTracker(mContext, editable);
            mEditable = editable;
        }

        updateSelectionState(selectionText, startOffset);
        Log.d(TAG, "logSelectionStarted");
        if (!logConditionCheck()) {
            return;
        }
        Log.d(TAG, "logSelectionStarted");
        logEvent(eventSelectionStarted(startOffset));
    }

    @Override
    public void logSelectionModified(
            String selectionText, int startOffset, SelectionClient.Result result) {
        Log.d(TAG, "logSelectionModified");
        if (!logConditionCheck()) {
            return;
        }
        Log.d(TAG, "logSelectionModified");

        int endOffset = startOffset + selectionText.length();
        Log.d(TAG, "logSelectionModified");
        if (result != null && result.textSelection != null) {
            logEvent(eventSelectionModified(startOffset, endOffset, result.textSelection));
        } else if (result != null && result.textClassification != null) {
            logEvent(eventSelectionModified(startOffset, endOffset, result.textClassification));
        } else {
            logEvent(eventSelectionModified(startOffset, endOffset));
        }
    }

    @Override
    public void logSelectionAction(String selectionText, int startOffset, @ActionType int action,
            SelectionClient.Result result) {
        if (!logConditionCheck()) {
            return;
        }

        Log.d(TAG, "logSelectionAction");
        int endOffset = startOffset + selectionText.length();
        if (result != null && result.textClassification != null) {
            logEvent(eventSelectionAction(
                    startOffset, endOffset, action, result.textClassification));
        } else {
            logEvent(eventSelectionAction(startOffset, endOffset, action));
        }
    }

    private void updateSelectionState(String selectionText, int startOffset) {
        mPreviousSelectionText = selectionText;
        mPreviousSelectionStartOffset = startOffset;
        mPreviousSelectionEndOffset = startOffset + selectionText.length();
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
    private boolean overlap(int al, int ar, int bl, int br) {
        if (al <= bl) {
            return bl < ar;
        }
        return br > al;
    }
}
