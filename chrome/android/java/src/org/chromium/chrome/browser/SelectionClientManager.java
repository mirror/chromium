// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.support.annotation.Nullable;
import android.view.textclassifier.TextClassifier;

import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.SmartSelectionClient;
import org.chromium.content_public.browser.SelectionClient;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/**
 * Manages the current {@link SelectionClient} instances, with support for 0-2 instances, and
 * is itself a {@link SelectionClient} instance.  This class supports one permanent instance for
 * Smart Text Selection, and one non-permanent instance for Contextual Search that can be added or
 * removed.
 * <p>
 * Usage: After being constructed this class knows if Smart Select is active or not, and can be used
 * as a {@link SelectionClient}.  If Smart Select is active it will pass calls to the Smart Select
 * Client, and if not then they will be ignored.
 * A non-permanent client may be added using
 * {@link #addContextualSearchSelectionClient(SelectionClient)} to connect to Contextual Search.
 * This client may be removed later using {@link #removeContextualSearchSelectionClient()}.
 */
public class SelectionClientManager implements SelectionClient {
    // Whether Smart Select is allowed to be enabled in Chrome.
    private static final boolean ALLOW_CHROME_SMART_SELECTION = true;

    private final boolean mIsSmartSelectionEnabledInChrome;

    /**
     * The single optional client supported directly by this class.
     * It may be null, the Smart Selection client, or our bridge between Smart Select and Contextual
     * Search.
     */
    private @Nullable SelectionClient mOptionalSelectionClient;
    private boolean mIsBridged;

    /**
     * Constructs an instance that is also a {@link SelectionClient} that does nothing until a
     * permanent client is set or a non-permanent client is added.
     *  */
    public SelectionClientManager(
            ContentViewCore contentViewCore, WindowAndroid windowAndroid, WebContents webContents) {
        if (ALLOW_CHROME_SMART_SELECTION) {
            mOptionalSelectionClient = SmartSelectionClient.create(
                    contentViewCore.getPopupControllerResultCallback(), windowAndroid, webContents);
        }
        mIsSmartSelectionEnabledInChrome = mOptionalSelectionClient != null;
    }

    /** @return Whether Smart Text Selection is currently enabled in Chrome. */
    public boolean isSmartSelectionEnabledInChrome() {
        return mIsSmartSelectionEnabledInChrome;
    }

    /**
     * Adds the given {@link SelectionClient} as an additional temporary instance that will be
     * notified of method calls.
     * @param contextualSearchSelectionClient An additional {@link SelectionClient} that should be
     *        notified of requests going forward, used by Contextual Search.
     */
    public SelectionClient addContextualSearchSelectionClient(
            SelectionClient contextualSearchSelectionClient) {
        assert contextualSearchSelectionClient != null;
        assert !mIsBridged : "No more than two selection client instances are supported!";
        if (mIsSmartSelectionEnabledInChrome) {
            mOptionalSelectionClient = new SelectionClientBridge(
                    mOptionalSelectionClient, contextualSearchSelectionClient);
            mIsBridged = true;
        } else {
            mOptionalSelectionClient = contextualSearchSelectionClient;
        }
        return this;
    }

    /**
     * Removes the current {@link SelectionClient} from the current instances that will be notified
     * of method calls.
     */
    public SelectionClient removeContextualSearchSelectionClient() {
        if (mIsSmartSelectionEnabledInChrome) {
            assert mIsBridged;
            SelectionClientBridge currentSelectionClientBridge =
                    (SelectionClientBridge) mOptionalSelectionClient;
            assert currentSelectionClientBridge != null;
            mOptionalSelectionClient = currentSelectionClientBridge.getSmartSelectionClient();
            mIsBridged = false;
        } else {
            assert !mIsBridged;
            mOptionalSelectionClient = null;
        }
        return this;
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
     * Bridges exactly two {@link SelectionClient} instances.  Both sides of the bridge receive
     * calls for most incoming calls to this instance.
     */
    private static class SelectionClientBridge implements SelectionClient {
        private final SelectionClient mSmartSelectionClient;
        private final SelectionClient mContextualSearchSelectionClient;

        /**
         * Constructs a bridge between the {@code smartSelectionClient} and the given
         * {@code contextualSearchSelectionClient}.  Method calls to this class are repeated to both
         * instances, with some exceptions: calls that return a value are only routed to the
         * {@code smartSelectionClient}.
         * @param smartSelectionClient The platform-dependent {@link SelectionClient}, which will be
         *        used as the dominant client, responsible for any results that need to be returned
         *        from method calls (typically a {@code SmartSelectionClient}).
         * @param contextualSearchSelectionClient A {@link SelectionClient} based on the
         *        {@code ContextualSearchManager}.
         */
        private SelectionClientBridge(SelectionClient smartSelectionClient,
                SelectionClient contextualSearchSelectionClient) {
            mSmartSelectionClient = smartSelectionClient;
            mContextualSearchSelectionClient = contextualSearchSelectionClient;
        }

        /**
         * @return The current Smart Selection client.
         */
        private SelectionClient getSmartSelectionClient() {
            return mSmartSelectionClient;
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
