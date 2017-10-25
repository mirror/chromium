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
    private final Class<?> mSelectionEventClass;

    static public SelectionEventProxyImpl create(Class<?> selectionEventClass) {
        return selectionEventClass == null ? null
                                           : new SelectionEventProxyImpl(selectionEventClass);
    }

    private SelectionEventProxyImpl(Class<?> selectionEventClass) {
        mSelectionEventClass = selectionEventClass;
    }

    // Reflection wrapper of SelectionEvent#selectionStarted(int)
    @Override
    public Object selectionStarted(int start) {
        try {
            Method selectionStartedMethod =
                    mSelectionEventClass.getMethod("selectionStarted", new Class[] {Integer.TYPE});
            return selectionStartedMethod.invoke(null, start);
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
            Method selectionModifiedMethod = mSelectionEventClass.getMethod(
                    "selectionModified", new Class[] {Integer.TYPE, Integer.TYPE});
            return selectionModifiedMethod.invoke(null, start, end);
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
            Method selectionModifiedMethod = mSelectionEventClass.getMethod("selectionModified",
                    new Class[] {Integer.TYPE, Integer.TYPE, TextClassification.class});
            return selectionModifiedMethod.invoke(null, start, end, classification);
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
            Method selectionModifiedMethod = mSelectionEventClass.getMethod(
                    "selectionModified", new Class[] {Integer.TYPE, Integer.TYPE});
            return selectionModifiedMethod.invoke(null, start, end);
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
            Method selectionActionMethod = mSelectionEventClass.getMethod(
                    "selectionAction", new Class[] {Integer.TYPE, Integer.TYPE, Integer.TYPE});
            return selectionActionMethod.invoke(null, start, end, actionType);
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
            Method selectionActionMethod = mSelectionEventClass.getMethod("selectionAction",
                    new Class[] {
                            Integer.TYPE, Integer.TYPE, Integer.TYPE, TextClassification.class});
            return selectionActionMethod.invoke(null, start, end, actionType, classification);
        } catch (Exception e) {
            // Avoid crashes due to logging.
            String msg = e.getMessage();
            if (DEBUG) Log.d(TAG, msg);
        }
        return null;
    }
}
