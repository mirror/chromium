// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.annotation.IntDef;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.content_public.browser.SelectionClient;
import org.chromium.content_public.browser.SelectionMetricsLogger;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;

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
    private static final boolean DEBUG = false;

    // Reflection classes, constructor and method.
    private static Class<?> sTrackerClass;
    private static Class<?> sSelectionEventClass;
    private static Constructor sTrackerConstructor;
    private static Method sLogEventMethod;
    private static boolean sReflectionFailed;

    private Context mContext;
    private Object mTracker;

    private SelectionEventProxy mSelectionEventProxy;
    private SelectionIndicesConverter mConverter;

    // ActionType, from SmartSelectionEventTracker.SelectionEvent class.
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ActionType.OVERTYPE, ActionType.COPY, ActionType.PASTE, ActionType.CUT,
            ActionType.SHARE, ActionType.SMART_SHARE, ActionType.DRAG, ActionType.ABANDON,
            ActionType.OTHER, ActionType.SELECT_ALL, ActionType.RESET})
    public @interface ActionType {
        /** User typed over the selection. */
        int OVERTYPE = 100;
        /** User copied the selection. */
        int COPY = 101;
        /** User pasted over the selection. */
        int PASTE = 102;
        /** User cut the selection. */
        int CUT = 103;
        /** User shared the selection. */
        int SHARE = 104;
        /** User clicked the textAssist menu item. */
        int SMART_SHARE = 105;
        /** User dragged+dropped the selection. */
        int DRAG = 106;
        /** User abandoned the selection. */
        int ABANDON = 107;
        /** User performed an action on the selection. */
        int OTHER = 108;

        /* Non-terminal actions. */
        /** User activated Select All */
        int SELECT_ALL = 200;
        /** User reset the smart selection. */
        int RESET = 201;
    }

    /**
     * SmartSelectionEventTracker.SelectionEvent class proxy. Having this interface for testing
     * purpose. It only has methods which are not requiring O APIs.
     */
    public static interface SelectionEventProxy {
        /**
         * Creates a SelectionEvent for selection started event.
         * @param start Start word index.
         */
        Object createSelectionStarted(int start);

        /**
         * Creates a SelectionEvent for selection modified event.
         * @param start Start word index.
         * @param end End word index.
         */
        Object createSelectionModified(int start, int end);

        /**
         * Creates a SelectionEvent for taking action on selection.
         * @param start Start word index.
         * @param end End word index.
         * @param actionType The action type defined in SelectionMetricsLogger.ActionType.
         */
        Object createSelectionAction(int start, int end, int actionType);
    }

    public static SmartSelectionMetricsLogger create(Context context) {
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.O || context == null
                || sReflectionFailed) {
            return null;
        }

        final String className = "android.view.textclassifier.logging.SmartSelectionEventTracker";
        if (sTrackerClass == null) {
            try {
                sTrackerClass = Class.forName(className);
                sSelectionEventClass = Class.forName(className + "$SelectionEvent");
                sTrackerConstructor =
                        sTrackerClass.getConstructor(new Class[] {Context.class, Integer.TYPE});
                sLogEventMethod =
                        sTrackerClass.getMethod("logEvent", new Class[] {sSelectionEventClass});
            } catch (ClassNotFoundException | NoSuchMethodException e) {
                if (DEBUG) Log.d(TAG, e.getMessage());
                sReflectionFailed = true;
                return null;
            }
        }
        return new SmartSelectionMetricsLogger(context);
    }

    private SmartSelectionMetricsLogger(Context context) {
        mContext = context;
        mSelectionEventProxy = SelectionEventProxyImpl.create(sSelectionEventClass);
        mConverter = new SelectionIndicesConverter();
    }

    @VisibleForTesting
    protected SmartSelectionMetricsLogger(SelectionEventProxy selectionEventProxy) {
        mSelectionEventProxy = selectionEventProxy;
        mConverter = new SelectionIndicesConverter();
    }

    public void logSelectionStarted(String selectionText, int startOffset, boolean editable) {
        mTracker = createTracker(mContext, editable);
        mConverter.updateSelectionState(selectionText, startOffset, /* force = */ true);
        mConverter.setInitialStartOffset(startOffset);

        if (DEBUG) Log.d(TAG, "logSelectionStarted");
        logEvent(mSelectionEventProxy.createSelectionStarted(0));
    }

    public void logSelectionModified(
            String selectionText, int startOffset, SelectionClient.Result result) {
        if (!mConverter.updateSelectionState(selectionText, startOffset, /* force = */ false)) {
            mTracker = null;
        }

        if (!hasSelectionStarted()) return;

        int endOffset = startOffset + selectionText.length();
        int[] indices = new int[2];
        final boolean valid = mConverter.getWordDelta(startOffset, endOffset, indices);

        if (DEBUG) Log.d(TAG, "logSelectionModified [" + indices[0] + ", " + indices[1] + ")");
        if (result != null && result.textSelection != null) {
            logEvent(
                    ((SelectionEventProxyImpl) mSelectionEventProxy)
                            .createSelectionModified(indices[0], indices[1], result.textSelection));
        } else if (result != null && result.textClassification != null) {
            logEvent(((SelectionEventProxyImpl) mSelectionEventProxy)
                             .createSelectionModified(
                                     indices[0], indices[1], result.textClassification));
        } else {
            logEvent(mSelectionEventProxy.createSelectionModified(indices[0], indices[1]));
        }

        if (!valid) mTracker = null;
    }

    public void logSelectionAction(String selectionText, int startOffset, @ActionType int action,
            SelectionClient.Result result) {
        if (!mConverter.updateSelectionState(selectionText, startOffset, /* force = */ false)) {
            mTracker = null;
        }
        if (!hasSelectionStarted()) return;

        int endOffset = startOffset + selectionText.length();
        int[] indices = new int[2];
        final boolean valid = mConverter.getWordDelta(startOffset, endOffset, indices);

        if (DEBUG) {
            Log.d(TAG,
                    "logSelectionAction [" + indices[0] + ", " + indices[1]
                            + ") ActionType = " + action);
        }

        if (result != null && result.textClassification != null) {
            logEvent(((SelectionEventProxyImpl) mSelectionEventProxy)
                             .createSelectionAction(
                                     indices[0], indices[1], action, result.textClassification));
        } else {
            logEvent(mSelectionEventProxy.createSelectionAction(indices[0], indices[1], action));
        }

        if (isTerminal(action) || !valid) mTracker = null;
    }

    private boolean hasSelectionStarted() {
        return mTracker != null && mSelectionEventProxy != null;
    }

    public Object createTracker(Context context, boolean editable) {
        try {
            return sTrackerConstructor.newInstance(
                    context, editable ? /* EDIT_WEBVIEW */ 4 : /* WEBVIEW */ 2);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            String msg = e.getMessage();
            if (DEBUG) Log.d(TAG, msg);
        }
        return null;
    }

    // Reflection wrapper of SmartSelectionEventTracker#logEvent(SelectionEvent)
    public void logEvent(Object selectionEvent) {
        if (selectionEvent == null) return;

        try {
            sLogEventMethod.invoke(mTracker, sSelectionEventClass.cast(selectionEvent));
        } catch (ClassCastException | ReflectiveOperationException e) {
            // Avoid crashes due to logging.
            if (DEBUG) Log.d(TAG, e.getMessage());
        }
    }

    public static boolean isTerminal(@ActionType int actionType) {
        switch (actionType) {
            case ActionType.OVERTYPE: // fall through
            case ActionType.COPY: // fall through
            case ActionType.PASTE: // fall through
            case ActionType.CUT: // fall through
            case ActionType.SHARE: // fall through
            case ActionType.SMART_SHARE: // fall through
            case ActionType.DRAG: // fall through
            case ActionType.ABANDON: // fall through
            case ActionType.OTHER: // fall through
                return true;
            default:
                return false;
        }
    }
}
