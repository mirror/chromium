// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.view.View.OnClickListener;
import android.view.textclassifier.TextClassifier;

import javax.annotation.Nullable;

/**
 * Interface to a content layer client that can process and modify selection text and supports
 * Smart Select.
 */
public interface SmartSelectionClient {
    /**
     * The result of the text analysis.
     */
    public static class Result {
        /**
         * The number of characters that the left boundary of the original
         * selection should be moved. Negative number means moving left.
         */
        public int startAdjust;

        /**
         * The number of characters that the right boundary of the original
         * selection should be moved. Negative number means moving left.
         */
        public int endAdjust;

        /**
         * Label for the suggested menu item.
         */
        public CharSequence label;

        /**
         * Icon for the suggested menu item.
         */
        public Drawable icon;

        /**
         * Intent for the suggested menu item.
         */
        public Intent intent;

        /**
         * OnClickListener for the suggested menu item.
         */
        public OnClickListener onClickListener;

        /**
         * A helper method that returns true if the result has both visual info
         * and an action so that, for instance, one can make a new menu item.
         */
        public boolean hasNamedAction() {
            return (label != null || icon != null) && (intent != null || onClickListener != null);
        }
    }

    /**
     * The interface that returns the result of the selected text analysis.
     */
    public interface ResultCallback {
        /**
         * The result is delivered with this method.
         */
        void onClassified(Result result);
    }

    /**
     * Notification that the web content selection has changed, regardless of the causal action.
     * @param selection The newly established selection.
     */
    void onSelectionChanged(String selection);

    /**
     * Notification that a user-triggered selection or insertion-related event has occurred.
     * @param eventType The selection event type, see {@link SelectionEventType}.
     * @param posXPix The x coordinate of the selection start handle.
     * @param posYPix The y coordinate of the selection start handle.
     */
    void onSelectionEvent(int eventType, float posXPix, float posYPix);

    /**
     * Notifies the SelectionClient that the Smart Select menu has been requested.
     * @param shouldSuggest Whether the SelectionClient should suggest and classify or just
     *        classify.
     * @return True if embedder should wait for a response before showing selection menu.
     */
    boolean requestSelectionPopupUpdates(boolean shouldSuggest);

    /**
     * Cancels any outstanding requests the embedder had previously requested using
     * SelectionClient.requestSelectionPopupUpdates().
     */
    void cancelAllRequests();

    /**
     * Sets the {@link TextClassifier} for the Smart Select text selection. Pass {@code null} to use
     * the system classifier.
     */
    default void setTextClassifier(@Nullable TextClassifier textClassifier) {}

    /**
     * Gets the {@link TextClassifier} that is used for the Smart Select text selection. If the
     * custom classifier has been set with #setTextClassifier, returns that object, otherwise
     * returns the system classifier.
     */
    default TextClassifier getTextClassifier() {
        return null;
    }

    /**
     * Returns the {@link TextClassifier} which has been set with #setTextClassifier, or
     * {@code null} if no custom classifier has been set.
     */
    default TextClassifier getCustomTextClassifier() {
        return null;
    }
}
