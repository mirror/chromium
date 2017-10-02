// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.textclassifier.TextClassifier;

import org.chromium.content_public.browser.SelectionClient;

import javax.annotation.Nullable;

/**
 * Manages the current {@link SelectionClient} instances, with support for 0-2 instances, and
 * is itself a {@link SelectionClient} instance.  This class supports one permanent instance that
 * can be set and one non-permanent instance that can be added or removed.
 * TODO(donnd): split SelectionClient SmartSelectionClient and ContextualSearchSelectionClient and
 * enforce setSelectionClient only on SmartSelectionClient and addSelectionClient only on
 * ContextualSearchSelectionClient.
 * <p>
 * Usage: After being constructed with the default constructor it will ignore selection client calls
 * until {@link #setSelectionClient(SelectionClient)} or
 * {@link #addSelectionClient(SelectionClient, Runnable)} is used to connect another instance.
 * <p>
 * A single-setup API is available in {@link #setSelectionClient(SelectionClient)} -- it sets an
 * optional permanent client that the caller cannot remove.
 * <p>
 * An add/remove API is available in {@link #addSelectionClient(SelectionClient, Runnable)} and
 * {@link #removeSelectionClient(SelectionClient)} that allows the caller to add and remove a
 * non-permanent selection client. This non-permanent {@link SelectionClient} can be suppressed when
 * the permanent {@link SelectionClient} is present.
 */
public class SelectionClientManager implements SelectionClient {
    private boolean mIsBridged;
    private @Nullable SelectionClient mOptionalSelectionClient;

    /**
     * Constructs an instance that is also a {@link SelectionClient} that does nothing until a
     * permanent client is set or a non-permanent client is added.
     *  */
    SelectionClientManager() {
        mOptionalSelectionClient = null;
    }

    /**
     * Sets the internal {@link SelectionClient} to the given permanent selectionClient, which can
     * cause this instance to start taking action in response to {@link SelectionClient} method
     * calls.  Once set, this permanent client cannot be removed.
     * @param permanentSelectionClient A {@link SelectionClient} or {@code null} for the selection
     *        client that should handle requests going forward.  Typically a
     *        {@code SmartSelectionClient} instance is used.
     */
    void setSelectionClient(@Nullable SelectionClient permanentSelectionClient) {
        mOptionalSelectionClient = permanentSelectionClient;
    }

    /**
     * Adds the given {@link SelectionClient} as an additional temporary instance that will be
     * notified of method calls.
     * @param selectionClientToAdd An additional {@link SelectionClient} that should be notified of
     *        requests going forward.  Typically a {@code ContextualSearchSelectionClient} instance
     *        is used.
     * @param suppressContextualSearchForSmartSelectRunnable A {@link Runnable} that will notify
     *        Contextual Search that it should suppress its UI, called when Smart Select is active.
     */
    void addSelectionClient(SelectionClient selectionClientToAdd,
            Runnable suppressContextualSearchForSmartSelectRunnable) {
        assert selectionClientToAdd != null;
        if (mOptionalSelectionClient == null) {
            mOptionalSelectionClient = selectionClientToAdd;
        } else {
            assert !mIsBridged : "No more than two selection client instances are supported!";
            mOptionalSelectionClient =
                    new SelectionClientBridge(mOptionalSelectionClient, selectionClientToAdd);
            mIsBridged = true;
            // Tell ContextualSearch to suppress it's UI for SmartSelect.
            suppressContextualSearchForSmartSelectRunnable.run();
        }
    }

    /**
     * Removes the given {@link SelectionClient} from the current instances that will be notified of
     * method calls.
     * @param selectionClientToRemove A {@link SelectionClient} that is currently handling requests
     *        that should no longer handle them.  Typically a
     *        {@code ContextualSearchSelectionClient} instance is used.
     */
    void removeSelectionClient(SelectionClient selectionClientToRemove) {
        if (mOptionalSelectionClient == selectionClientToRemove) {
            mOptionalSelectionClient = null;
        } else {
            assert mIsBridged;
            SelectionClientBridge selectionClientBridge =
                    (SelectionClientBridge) mOptionalSelectionClient;
            assert selectionClientBridge != null;
            assert selectionClientToRemove
                    == selectionClientBridge.getContextualSearchSelectionClient();
            mOptionalSelectionClient = selectionClientBridge.getSmartSelectionClient();
            mIsBridged = false;
        }
    }

    @Override
    public void onSelectionChanged(String selection) {
        if (mOptionalSelectionClient != null) {
            mOptionalSelectionClient.onSelectionChanged(selection);
        }
    }

    @Override
    public void onSelectionEvent(int eventType, float posXPix, float posYPix) {
        if (mOptionalSelectionClient != null) {
            mOptionalSelectionClient.onSelectionEvent(eventType, posXPix, posYPix);
        }
    }

    @Override
    public void showUnhandledTapUIIfNeeded(int x, int y) {
        if (mOptionalSelectionClient != null) {
            mOptionalSelectionClient.showUnhandledTapUIIfNeeded(x, y);
        }
    }

    @Override
    public void selectWordAroundCaretAck(boolean didSelect, int startAdjust, int endAdjust) {
        if (mOptionalSelectionClient != null) {
            mOptionalSelectionClient.selectWordAroundCaretAck(didSelect, startAdjust, endAdjust);
        }
    }

    @Override
    public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
        return mOptionalSelectionClient != null
                ? mOptionalSelectionClient.requestSelectionPopupUpdates(shouldSuggest)
                : false;
    }

    @Override
    public void cancelAllRequests() {
        if (mOptionalSelectionClient != null) mOptionalSelectionClient.cancelAllRequests();
    }

    @Override
    public void setTextClassifier(TextClassifier textClassifier) {
        if (mOptionalSelectionClient != null) {
            mOptionalSelectionClient.setTextClassifier(textClassifier);
        }
    }

    @Override
    public TextClassifier getTextClassifier() {
        return mOptionalSelectionClient != null ? mOptionalSelectionClient.getTextClassifier()
                                                : null;
    }

    @Override
    public TextClassifier getCustomTextClassifier() {
        return mOptionalSelectionClient != null ? mOptionalSelectionClient.getCustomTextClassifier()
                                                : null;
    }

    /**
     * Bridges exactly two {@link SelectionClient} instances.  The instance for Contextual Search
     * will not be notified of long-press based selection events.
     */
    private class SelectionClientBridge implements SelectionClient {
        private final SelectionClient mSmartSelectionClient;
        private final SelectionClient mContextualSearchSelectionClient;

        /**
         * Constructs a bridge between the {@code previousSelectionClient} and the given
         * {@code contextualSearchSelectionClient}.  Method calls to this class are repeated to both
         * instances, with some exceptions.
         * @param smartSelectionClient The platform-dependent {@link SelectionClient}, which will be
         *        used as the dominant client, responsible for any results that need to be returned
         *        from method calls (typically a SmartSelectionClient).
         * @param contextualSearchSelectionClient A {@link SelectionClient} based on the
         *        {@code ContextualSearchManager}.
         */
        private SelectionClientBridge(SelectionClient smartSelectionClient,
                SelectionClient contextualSearchSelectionClient) {
            mSmartSelectionClient = smartSelectionClient;
            mContextualSearchSelectionClient = contextualSearchSelectionClient;
        }

        private SelectionClient getSmartSelectionClient() {
            return mSmartSelectionClient;
        }

        private SelectionClient getContextualSearchSelectionClient() {
            return mContextualSearchSelectionClient;
        }

        @Override
        public void onSelectionChanged(String selection) {
            mSmartSelectionClient.onSelectionChanged(selection);
            mContextualSearchSelectionClient.onSelectionChanged(selection);
        }

        @Override
        public void onSelectionEvent(int eventType, float posXPix, float posYPix) {
            mSmartSelectionClient.onSelectionEvent(eventType, posXPix, posYPix);
            mContextualSearchSelectionClient.onSelectionEvent(eventType, posXPix, posYPix);
        }

        @Override
        public void showUnhandledTapUIIfNeeded(int x, int y) {
            mSmartSelectionClient.showUnhandledTapUIIfNeeded(x, y);
            mContextualSearchSelectionClient.showUnhandledTapUIIfNeeded(x, y);
        }

        @Override
        public void selectWordAroundCaretAck(boolean didSelect, int startAdjust, int endAdjust) {
            mSmartSelectionClient.selectWordAroundCaretAck(didSelect, startAdjust, endAdjust);
            mContextualSearchSelectionClient.selectWordAroundCaretAck(
                    didSelect, startAdjust, endAdjust);
        }

        @Override
        public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
            return mSmartSelectionClient.requestSelectionPopupUpdates(shouldSuggest);
        }

        @Override
        public void cancelAllRequests() {
            mSmartSelectionClient.cancelAllRequests();
            mContextualSearchSelectionClient.cancelAllRequests();
        }

        @Override
        public void setTextClassifier(TextClassifier textClassifier) {
            mSmartSelectionClient.setTextClassifier(textClassifier);
            mContextualSearchSelectionClient.setTextClassifier(textClassifier);
        }

        @Override
        public TextClassifier getTextClassifier() {
            return mSmartSelectionClient.getTextClassifier();
        }

        @Override
        public TextClassifier getCustomTextClassifier() {
            return mSmartSelectionClient.getCustomTextClassifier();
        }
    }
}
