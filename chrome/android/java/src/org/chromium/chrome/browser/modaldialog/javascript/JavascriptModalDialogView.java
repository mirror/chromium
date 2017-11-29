// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog.javascript;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.modaldialog.ModalDialogView;

/**
 * The JavaScript dialog that is either app modal or tab modal.
 */
public class JavascriptModalDialogView extends ModalDialogView {
    private final TextView mMessageView;
    private final EditText mPromptEditText;
    private final CheckBox mSuppressCheckBox;
    private final View mScrollView;

    public JavascriptModalDialogView(ModalDialogView.Controller controller, String message,
            String promptText, boolean shouldShowSuppressCheckBox) {
        super(controller);

        LayoutInflater inflater = LayoutInflater.from(getContext());
        View customLayout = inflater.inflate(R.layout.js_modal_dialog, null);
        mScrollView = customLayout.findViewById(R.id.js_modal_dialog_scroll_view);
        mMessageView = customLayout.findViewById(R.id.js_modal_dialog_message);
        mPromptEditText = customLayout.findViewById(R.id.js_modal_dialog_prompt);
        mSuppressCheckBox = customLayout.findViewById(R.id.suppress_js_modal_dialogs);

        mMessageView.setText(message);
        setCustomView(customLayout);
        setPromptText(promptText);
        setSuppressCheckBoxVisibility(shouldShowSuppressCheckBox);
    }

    @Override
    protected void prepareBeforeShow() {
        super.prepareBeforeShow();
        // If the message is null or empty do not display the message text view.
        // Hide parent scroll view instead of text view in order to prevent ui discrepancies.
        if (mMessageView.getText().length() == 0) {
            mScrollView.setVisibility(View.GONE);
        } else {
            // TODO(huayinz): See if View#canScrollVertictically() can be used for checking if
            // scrollView is scrollable.
            mScrollView.addOnLayoutChangeListener(
                    (v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> {
                        boolean isScrollable =
                                v.getMeasuredHeight() - v.getPaddingTop() - v.getPaddingBottom()
                                < ((ViewGroup) v).getChildAt(0).getMeasuredHeight();
                        v.setFocusable(isScrollable);
                    });
        }
    }

    /**
     * @param promptText Prompt text for prompt dialog. If null, prompt text is not visible.
     */
    private void setPromptText(String promptText) {
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
     * @param visible Whether the suppress check box should be visible. The check box should only
     *                be set visible if applicable for app modal JavaScript dialog.
     */
    private void setSuppressCheckBoxVisibility(boolean visible) {
        mSuppressCheckBox.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    /**
     * @return Whether the suppress check box is checked by user.
     */
    public boolean isSuppressCheckBoxChecked() {
        return mSuppressCheckBox.isChecked();
    }
}
