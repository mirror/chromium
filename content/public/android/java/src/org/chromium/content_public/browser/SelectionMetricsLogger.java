// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.content.browser.SmartSelectionMetricsLogger;

/**
 * Interface for selection event logging.
 */
public interface SelectionMetricsLogger {
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
    void logSelectionAction(int start, int end, int action, SelectionClient.Result result);

    /** Creates a {@link SmartSelectionLogger} instance. */
    public static SmartSelectionMetricsLogger createSmartSelectionMetricsLogger(
            WebContents webContents) {
        return SmartSelectionMetricsLogger.create(webContents);
    }
}
