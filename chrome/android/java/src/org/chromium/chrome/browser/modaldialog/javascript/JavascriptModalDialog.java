// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog.javascript;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.modaldialog.ModalDialog;

/**
 * The JavaScript dialog that is either app modal or tab modal.
 */
public class JavascriptModalDialog extends ModalDialog {
    private final View mCustomLayout;
    private final TextView mMessageView;
    private final EditText mPromptEditText;
    private final CheckBox mSuppressCheckBox;

    public JavascriptModalDialog(
            Activity ownerActivity, ModalDialog.Controller controller, boolean isTabModal) {
        super(ownerActivity, controller, isTabModal);

        LayoutInflater inflater = LayoutInflater.from(mActivity);
        mCustomLayout = inflater.inflate(R.layout.js_modal_dialog, null);
        mMessageView = mCustomLayout.findViewById(R.id.js_modal_dialog_message);
        mPromptEditText = mCustomLayout.findViewById(R.id.js_modal_dialog_prompt);
        mSuppressCheckBox = mCustomLayout.findViewById(R.id.suppress_js_modal_dialogs);

        setCustomView(mCustomLayout);
    }

    @Override
    protected void prepareBeforeShow() {
        super.prepareBeforeShow();
        // If the message is null or empty do not display the message text view.
        // Hide parent scroll view instead of text view in order to prevent ui discrepancies.
        View scrollView = mCustomLayout.findViewById(R.id.js_modal_dialog_scroll_view);
        if (mMessageView.getText().length() == 0) {
            scrollView.setVisibility(View.GONE);
        } else {
            // TODO(huayinz): See if View#canScrollVertictically() can be used for checking if
            // scrollView is scrollable.
            scrollView.addOnLayoutChangeListener(
                    (v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> {
                        boolean isScrollable =
                                v.getMeasuredHeight() - v.getPaddingTop() - v.getPaddingBottom()
                                < ((ViewGroup) v).getChildAt(0).getMeasuredHeight();
                        v.setFocusable(isScrollable);
                    });
        }
    }

    @Override
    public void setMessage(String message) {
        mMessageView.setText(message);
    }

    /**
     * @param promptText Prompt text for prompt dialog. If null, prompt text is not visible.
     */
    public void setPromptText(String promptText) {
        if (promptText == null) return;
        mPromptEditText.setVisibility(View.VISIBLE);

        if (promptText.length() > 0) {
            mPromptEditText.setText(promptText);
            mPromptEditText.selectAll();
        }
    }

    /**
     * @return The prompt text edited by user.
     */
    public String getPromptText() {
        return mPromptEditText.getText().toString();
    }

    /**
     * @param visible Whether the suppress check box should be visible.
     */
    public void setSuppressCheckBoxVisibility(boolean visible) {
        mSuppressCheckBox.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    /**
     * @return Whether the suppress check box is checked by user.
     */
    public boolean isSuppressCheckBoxChecked() {
        return mSuppressCheckBox.isChecked();
    }
}
