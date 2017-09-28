// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.os.Build;
import android.support.annotation.IntDef;
import android.text.TextUtils;
import android.view.textclassifier.TextClassifier;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import javax.annotation.Nullable;

/**
 * A class that controls Smart Text selection. Smart Text selection automatically augments the
 * selected boundaries and classifies the selected text based on the context.
 * This class requests the selection together with its surrounding text from the focused frame and
 * sends it to SmartSelectionProvider which does the classification itself.
 */
@JNINamespace("content")
public class SmartSelectionClientImpl implements SmartSelectionClient {
    @IntDef({CLASSIFY, SUGGEST_AND_CLASSIFY})
    @Retention(RetentionPolicy.SOURCE)
    private @interface RequestType {}

    // Request to obtain the type (e.g. phone number, e-mail address) and the most
    // appropriate operation for the selected text.
    private static final int CLASSIFY = 0;

    // Request to obtain the type (e.g. phone number, e-mail address), the most
    // appropriate operation for the selected text and a better selection boundaries.
    private static final int SUGGEST_AND_CLASSIFY = 1;

    // The maximal number of characters on the left and on the right from the current selection.
    // Used for surrounding text request.
    private static final int NUM_EXTRA_CHARS = 240;

    // Is smart selection enabled?
    private static boolean sEnabled;

    private long mNativeSmartSelectionClientImpl;
    private SmartSelectionProvider mProvider;
    private ResultCallback mCallback;

    public static void setEnabled(boolean enabled) {
        sEnabled = enabled;
    }

    /**
     * Creates the SmartSelectionClientImpl. Returns null in case SmartSelectionProvider does not
     * exist in the system.
     */
    public static SmartSelectionClientImpl create(
            ResultCallback callback, WindowAndroid windowAndroid, WebContents webContents) {
        if (!sEnabled) return null;
        SmartSelectionProvider provider = new SmartSelectionProvider(callback, windowAndroid);
        return new SmartSelectionClientImpl(provider, callback, webContents);
    }

    private SmartSelectionClientImpl(
            SmartSelectionProvider provider, ResultCallback callback, WebContents webContents) {
        assert Build.VERSION.SDK_INT >= Build.VERSION_CODES.O;
        mProvider = provider;
        mCallback = callback;
        mNativeSmartSelectionClientImpl = nativeInit(webContents);
    }

    @CalledByNative
    private void onNativeSideDestroyed(long nativeSmartSelectionClientImpl) {
        assert nativeSmartSelectionClientImpl == mNativeSmartSelectionClientImpl;
        mNativeSmartSelectionClientImpl = 0;
        mProvider.cancelAllRequests();
    }

    // SelectionClient implementation
    @Override
    public void onSelectionChanged(String selection) {}

    @Override
    public void onSelectionEvent(int eventType, float posXPix, float posYPix) {}

    @Override
    public boolean requestSelectionPopupUpdates(boolean shouldSuggest) {
        requestSurroundingText(shouldSuggest ? SUGGEST_AND_CLASSIFY : CLASSIFY);
        return true;
    }

    @Override
    public void cancelAllRequests() {
        if (mNativeSmartSelectionClientImpl != 0) {
            nativeCancelAllRequests(mNativeSmartSelectionClientImpl);
        }

        mProvider.cancelAllRequests();
    }

    @Override
    public void setTextClassifier(@Nullable TextClassifier textClassifier) {
        mProvider.setTextClassifier(textClassifier);
    }

    @Override
    public TextClassifier getTextClassifier() {
        return mProvider.getTextClassifier();
    }

    @Override
    public TextClassifier getCustomTextClassifier() {
        return mProvider.getCustomTextClassifier();
    }

    private void requestSurroundingText(@RequestType int callbackData) {
        if (mNativeSmartSelectionClientImpl == 0) {
            onSurroundingTextReceived(callbackData, "", 0, 0);
            return;
        }

        nativeRequestSurroundingText(
                mNativeSmartSelectionClientImpl, NUM_EXTRA_CHARS, callbackData);
    }

    @CalledByNative
    private void onSurroundingTextReceived(
            @RequestType int callbackData, String text, int start, int end) {
        if (!textHasValidSelection(text, start, end)) {
            mCallback.onClassified(new Result());
            return;
        }

        switch (callbackData) {
            case SUGGEST_AND_CLASSIFY:
                mProvider.sendSuggestAndClassifyRequest(text, start, end, null);
                break;

            case CLASSIFY:
                mProvider.sendClassifyRequest(text, start, end, null);
                break;

            default:
                assert false : "Unexpected callback data";
                break;
        }
    }

    private boolean textHasValidSelection(String text, int start, int end) {
        return !TextUtils.isEmpty(text) && 0 <= start && start < end && end <= text.length();
    }

    private native long nativeInit(WebContents webContents);
    private native void nativeRequestSurroundingText(
            long nativeSmartSelectionClientImpl, int numExtraCharacters, int callbackData);
    private native void nativeCancelAllRequests(long nativeSmartSelectionClientImpl);
}
