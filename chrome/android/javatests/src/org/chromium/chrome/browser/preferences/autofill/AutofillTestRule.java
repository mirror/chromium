// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.widget.EditText;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.payments.ui.EditorDialog;
import org.chromium.chrome.browser.payments.ui.PaymentRequestUI;
import org.chromium.chrome.browser.payments.ui.PaymentRequestUI.PaymentRequestObserverForTest;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;

import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * Custom ChromeBrowserTestRule to test Autofill.
 */
class AutofillTestRule extends ChromeBrowserTestRule implements PaymentRequestObserverForTest {
    final CallbackHelper mClickUpdate;
    final CallbackHelper mEditorTextUpdate;
    final CallbackHelper mPrefereneUpdate;

    private EditorDialog mEditorDialog;

    AutofillTestRule() {
        mClickUpdate = new CallbackHelper();
        mEditorTextUpdate = new CallbackHelper();
        mPrefereneUpdate = new CallbackHelper();
        AutofillProfilesFragment.setObserverForTest(AutofillTestRule.this);
    }

    protected void setTextInEditorAndWait(final String[] values)
            throws InterruptedException, TimeoutException {
        int callCount = mEditorTextUpdate.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            List<EditText> fields = mEditorDialog.getEditableTextFieldsForTest();
            for (int i = 0; i < values.length; i++) {
                fields.get(i).setText(values[i]);
            }
        });
        mEditorTextUpdate.waitForCallback(callCount);
    }

    protected void clickInEditorAndWait(final int resourceId)
            throws InterruptedException, TimeoutException {
        int callCount = mClickUpdate.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                (Runnable) () -> mEditorDialog.findViewById(resourceId).performClick());
        mClickUpdate.waitForCallback(callCount);
    }

    protected void waitForThePreferenceUpdate() throws InterruptedException, TimeoutException {
        int callCount = mPrefereneUpdate.getCallCount();
        mPrefereneUpdate.waitForCallback(callCount);
    }

    protected void setEditorDialog(EditorDialog editorDialog) {
        mEditorDialog = editorDialog;
    }

    @Override
    public void onPaymentRequestDismiss() {
        ThreadUtils.assertOnUiThread();
        mPrefereneUpdate.notifyCalled();
    }

    @Override
    public void onPaymentRequestEditorTextUpdate() {
        ThreadUtils.assertOnUiThread();
        mEditorTextUpdate.notifyCalled();
    }
    @Override
    public void onPaymentRequestReadyToEdit() {
        ThreadUtils.assertOnUiThread();
        mClickUpdate.notifyCalled();
    }

    @Override
    public void onPaymentRequestEditorValidationError() {}

    @Override
    public void onPaymentRequestReadyForInput(PaymentRequestUI ui) {}

    @Override
    public void onPaymentRequestSelectionChecked(PaymentRequestUI ui) {}

    @Override
    public void onPaymentRequestReadyToPay(PaymentRequestUI ui) {}

    @Override
    public void onPaymentRequestResultReady(PaymentRequestUI ui) {}
}
