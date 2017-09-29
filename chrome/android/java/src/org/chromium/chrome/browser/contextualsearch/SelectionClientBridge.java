// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.view.textclassifier.TextClassifier;

import org.chromium.content.browser.SelectionClient;

/**
 * A simple bridge that spans two {@link SelectionClient} implementations by just broadcasting
 * messages to both.<p>
 * Some methods return a result, and this implementation just returns the result from the
 * platform-dependent client (currently the "Smart Select" client).
 */
public class SelectionClientBridge implements SelectionClient {
    private final SelectionClient mContextualSearchSelectionClient;
    private final SelectionClient mSmartSelectClient;

    /**
     * Constructs a bridge between the {@code previousSelectionClient} and the given
     * {@code contextualSearchSelectionClient}.  Method calls to this class are repeated to both
     * instances.
     * @param platformSelectionClient The platform-dependent {@link SelectionClient}, which will be
     *        used as the dominant client, responsible for any results that need to be returned from
     *        method calls.
     * @param contextualSearchSelectionClient A {@link SelectionClient} based on the
     *        {@link ContextualSearchManager}.
     */
    SelectionClientBridge(SelectionClient platformSelectionClient,
            SelectionClient contextualSearchSelectionClient) {
        mContextualSearchSelectionClient = contextualSearchSelectionClient;
        assert platformSelectionClient != null;
        mSmartSelectClient = platformSelectionClient;
    }

    @Override
    public void onSelectionChanged(String selection) {
        mSmartSelectClient.onSelectionChanged(selection);
        mContextualSearchSelectionClient.onSelectionChanged(selection);
    }

    @Override
    public void onSelectionEvent(int eventType, float posXPix, float posYPix) {
        mSmartSelectClient.onSelectionEvent(eventType, posXPix, posYPix);
        mContextualSearchSelectionClient.onSelectionEvent(eventType, posXPix, posYPix);
    }

    @Override
    public void showUnhandledTapUIIfNeeded(int x, int y) {
        mSmartSelectClient.showUnhandledTapUIIfNeeded(x, y);
        mContextualSearchSelectionClient.showUnhandledTapUIIfNeeded(x, y);
    }

    @Override
    public void selectWordAroundCaretAck(boolean didSelect, int startAdjust, int endAdjust) {
        mSmartSelectClient.selectWordAroundCaretAck(didSelect, startAdjust, endAdjust);
        mContextualSearchSelectionClient.selectWordAroundCaretAck(
                didSelect, startAdjust, endAdjust);
    }

    @Override
    public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
        return mSmartSelectClient.requestSelectionPopupUpdates(shouldSuggest);
    }

    @Override
    public void cancelAllRequests() {
        mSmartSelectClient.cancelAllRequests();
        mContextualSearchSelectionClient.cancelAllRequests();
    }

    @Override
    public void setTextClassifier(TextClassifier textClassifier) {
        mSmartSelectClient.setTextClassifier(textClassifier);
        mContextualSearchSelectionClient.setTextClassifier(textClassifier);
    }

    @Override
    public TextClassifier getTextClassifier() {
        return mSmartSelectClient.getTextClassifier();
    }

    @Override
    public TextClassifier getCustomTextClassifier() {
        return mSmartSelectClient.getCustomTextClassifier();
    }
}
