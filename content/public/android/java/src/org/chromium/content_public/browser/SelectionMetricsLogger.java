// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import android.support.annotation.IntDef;

import org.chromium.content.browser.SmartSelectionMetricsLogger;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Interface for selection event logging.
 */
public interface SelectionMetricsLogger {
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
     * Log selection start event.
     * @param selectionText Current selection text.
     * @param startOffset Offset of the current selection.
     * @param editable If the current selection comes from an editable text area.
     */
    void logSelectionStarted(String selectionText, int startOffset, boolean editable);

    /**
     * Log selection modified event.
     * @param selectionText Current selection text.
     * @param startOffset Offset of the current selection.
     * @param result The current selection classification result.
     */
    void logSelectionModified(String selectionText, int startOffset, SelectionClient.Result result);

    /**
     * Log selection related action event.
     * @param start Start offset of the current selection.
     * @param end End offset of the current selection.
     * @param action Action id of the action.
     * @param result The current selection classification result.
     */
    void logSelectionAction(String selectionText, int startOffset, @ActionType int action,
            SelectionClient.Result result);

    /** Creates a {@link SmartSelectionLogger} instance. */
    public static SmartSelectionMetricsLogger createSmartSelectionMetricsLogger(
            WebContents webContents) {
        return SmartSelectionMetricsLogger.create(webContents);
    }
}
