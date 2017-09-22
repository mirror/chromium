// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.text.TextUtils;
import android.view.textclassifier.TextClassifier;
import org.chromium.content.browser.SelectionClient;
import org.chromium.ui.touch_selection.SelectionEventType;

public class SelectionClientBridge implements SelectionClient {
    private final SelectionClient mContextualSearchSelectionClient;
    private final SelectionClient mDefaultClient;

    private boolean mIsContextualSearchClientActive;
    private SelectionClient mCurrentClient;

    SelectionClientBridge(
            SelectionClient defaultClient, SelectionClient contextualSearchSelectionClient) {
        mContextualSearchSelectionClient = contextualSearchSelectionClient;
        mDefaultClient = defaultClient != null ? defaultClient : contextualSearchSelectionClient;

        mCurrentClient = mDefaultClient;
    }

    @Override
    public void onSelectionChanged(String selection) {
        mCurrentClient.onSelectionChanged(selection);

        // When the selection is cleared, revert to the default client.
        if (TextUtils.isEmpty(selection)) {
            mIsContextualSearchClientActive = false;
            mCurrentClient = mDefaultClient;
            System.out.println("ctxs onSelectionChanged mDefaultClient");
        }
    }

    @Override
    public void onSelectionEvent(int eventType, float posXPix, float posYPix) {
        mCurrentClient.onSelectionEvent(eventType, posXPix, posYPix);

        if (eventType == SelectionEventType.SELECTION_HANDLES_CLEARED) {
            mIsContextualSearchClientActive = false;
            mCurrentClient = mDefaultClient;
            System.out.println("ctxs onSelectionEvent default Client");
        }
    }

    @Override
    public void showUnhandledTapUIIfNeeded(int x, int y) {
        System.out.println("ctxs showUnhandledTapUIIfNeeded.");
        // When a Tap happens, engage the CS client.
        mIsContextualSearchClientActive = true;
        mCurrentClient = mContextualSearchSelectionClient;
        System.out.println("ctxs onSelectionChanged CS Client");
        mCurrentClient.showUnhandledTapUIIfNeeded(x, y);
    }

    @Override
    public void selectWordAroundCaretAck(boolean didSelect, int startAdjust, int endAdjust) {
        mCurrentClient.selectWordAroundCaretAck(didSelect, startAdjust, endAdjust);
    }

    @Override
    public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
        return mCurrentClient.requestSelectionPopupUpdates(shouldSuggest);
    }

    @Override
    public void cancelAllRequests() {
        mCurrentClient.cancelAllRequests();
    }

    @Override
    public void setTextClassifier(TextClassifier textClassifier) {
        mCurrentClient.setTextClassifier(textClassifier);
    }

    @Override
    public TextClassifier getTextClassifier() {
        return mCurrentClient.getTextClassifier();
    }

    @Override
    public TextClassifier getCustomTextClassifier() {
        return mCurrentClient.getCustomTextClassifier();
    }
}
