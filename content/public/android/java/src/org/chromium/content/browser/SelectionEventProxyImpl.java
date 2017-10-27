// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.annotation.TargetApi;
import android.os.Build;
import android.support.annotation.NonNull;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextSelection;

import org.chromium.base.Log;

import java.lang.reflect.Method;

/**
 * A wrapper class for Android SmartSelectionEventTracker.SelectionEvent.
 * SmartSelectionEventTracker.SelectionEvent is a hidden class, need reflection to access to it.
 */
@TargetApi(Build.VERSION_CODES.O)
public class SelectionEventProxyImpl implements SmartSelectionMetricsLogger.SelectionEventProxy {
    private static final String TAG = "SmartSelectionLogger";
    private static final boolean DEBUG = false;

    // Methods IDs
    private static final int SELECTION_STARTED = 0;
    private static final int SELECTION_MODIFIED = 1;
    private static final int SELECTION_MODIFIED_CLASSIFICATION = 2;
    private static final int SELECTION_MODIFIED_SELECTION = 3;
    private static final int SELECTION_ACTION = 4;
    private static final int SELECTION_ACTION_CLASSIFICATION = 5;

    private static boolean sReflectionFailed = false;
    private static final String[] sMethodNames = {"selectionStarted", "selectionModified",
            "selectionModified", "selectionModified", "selectionAction", "selectionAction"};
    private static final Class[][] sParams = {new Class[] {Integer.TYPE},
            new Class[] {Integer.TYPE, Integer.TYPE},
            new Class[] {Integer.TYPE, Integer.TYPE, TextClassification.class},
            new Class[] {Integer.TYPE, Integer.TYPE, TextSelection.class},
            new Class[] {Integer.TYPE, Integer.TYPE, Integer.TYPE},
            new Class[] {Integer.TYPE, Integer.TYPE, Integer.TYPE, TextClassification.class}};
    private static Method[] sMethods = new Method[6];

    static public SelectionEventProxyImpl create(Class<?> selectionEventClass) {
        if (sReflectionFailed || selectionEventClass == null) return null;
        for (int i = 0; i < sMethodNames.length; ++i) {
            try {
                sMethods[i] = selectionEventClass.getMethod(sMethodNames[i], sParams[i]);
            } catch (Exception e) {
                if (DEBUG) Log.d(TAG, e.getMessage());
                sReflectionFailed = true;
                return null;
            }
        }
        return new SelectionEventProxyImpl();
    }

    private SelectionEventProxyImpl() {}

    // Reflection wrapper of SelectionEvent#selectionStarted(int)
    @Override
    public Object selectionStarted(int start) {
        try {
            return sMethods[SELECTION_STARTED].invoke(null, start);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            String msg = e.getMessage();
            if (DEBUG) Log.d(TAG, msg);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionModified(int, int)
    @Override
    public Object selectionModified(int start, int end) {
        try {
            return sMethods[SELECTION_MODIFIED].invoke(null, start, end);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            String msg = e.getMessage();
            if (DEBUG) Log.d(TAG, msg);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionModified(int, int, TextClassification)
    public Object selectionModified(
            int start, int end, @NonNull TextClassification classification) {
        try {
            return sMethods[SELECTION_MODIFIED_CLASSIFICATION].invoke(
                    null, start, end, classification);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            String msg = e.getMessage();
            if (DEBUG) Log.d(TAG, msg);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionModified(int, int, TextSelection)
    public Object selectionModified(int start, int end, @NonNull TextSelection selection) {
        try {
            return sMethods[SELECTION_MODIFIED_SELECTION].invoke(null, start, end, selection);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            String msg = e.getMessage();
            if (DEBUG) Log.d(TAG, msg);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionAction(int, int, int)
    @Override
    public Object selectionAction(int start, int end, int actionType) {
        try {
            return sMethods[SELECTION_ACTION].invoke(null, start, end, actionType);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            String msg = e.getMessage();
            if (DEBUG) Log.d(TAG, msg);
        }
        return null;
    }

    // Reflection wrapper of SelectionEvent#selectionAction(int, int, int, TextClassification)
    public Object selectionAction(
            int start, int end, int actionType, @NonNull TextClassification classification) {
        try {
            return sMethods[SELECTION_ACTION_CLASSIFICATION].invoke(
                    null, start, end, actionType, classification);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            String msg = e.getMessage();
            if (DEBUG) Log.d(TAG, msg);
        }
        return null;
    }
}
