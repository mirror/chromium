// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import android.content.Context;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.text.TextUtils;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Generic builder for app modal or tab modal alert dialogs. Tab Modal dialog is available only when
 * its owner activity is instance of {@link ChromeTabbedActivity}.
 */
public class ModalDialogView implements View.OnClickListener {
    /**
     * Interface that controls the actions on the modal dialog.
     */
    public interface Controller {
        /**
         * Handle click event of the buttons on the dialog.
         * @param buttonType The type of the button.
         */
        void onClick(@ButtonType int buttonType);

        /**
         * Handle dismiss event when dialog is not dismissed by actions on the dialog such as back
         * press, and on tab modal dialog, tab switcher button click.
         */
        void onCancel();
    }

    @IntDef({BUTTON_POSITIVE, BUTTON_NEGATIVE})
    @Retention(RetentionPolicy.SOURCE)
    public @interface ButtonType {}
    public static final int BUTTON_POSITIVE = 0;
    public static final int BUTTON_NEGATIVE = 1;

    private final Controller mController;
    private final Context mContext;

    private View mCustom;

    private View mDialogView;
    private TextView mTitleView;
    private TextView mMessageView;
    private ViewGroup mCustomView;
    private Button mPositiveButton;
    private Button mNegativeButton;

    /**
     * Constructor for initializing controller and views.
     * @param controller The controller for this dialog.
     */
    public ModalDialogView(@NonNull Controller controller) {
        mController = controller;
        mContext = new ContextThemeWrapper(
                ContextUtils.getApplicationContext(), R.style.ModalDialogTheme);
        initViews();
    }

    /**
     * Initialize views for the contents of the dialog.
     */
    private void initViews() {
        mDialogView = LayoutInflater.from(mContext).inflate(R.layout.modal_dialog_view, null);
        mTitleView = mDialogView.findViewById(R.id.title);
        mMessageView = mDialogView.findViewById(R.id.message);
        mCustomView = mDialogView.findViewById(R.id.custom);
        mPositiveButton = mDialogView.findViewById(android.R.id.button1);
        mNegativeButton = mDialogView.findViewById(android.R.id.button2);
    }

    @Override
    public void onClick(View view) {
        if (view == mPositiveButton) {
            mController.onClick(BUTTON_POSITIVE);
        } else if (view == mNegativeButton) {
            mController.onClick(BUTTON_NEGATIVE);
        }
    }

    /**
     * Prepare the contents before showing the dialog.
     */
    protected void prepareBeforeShow() {
        if (TextUtils.isEmpty(mTitleView.getText())) mTitleView.setVisibility(View.GONE);
        if (TextUtils.isEmpty(mMessageView.getText())) {
            ((View) mMessageView.getParent()).setVisibility(View.GONE);
        }
        if (mCustom != null) {
            if (mCustom.getParent() != null) ((ViewGroup) mCustom.getParent()).removeView(mCustom);
            mCustomView.addView(mCustom);
        } else {
            mCustomView.setVisibility(View.GONE);
        }
        if (mPositiveButton.getText().length() == 0) mPositiveButton.setVisibility(View.GONE);
        if (mNegativeButton.getText().length() == 0) mNegativeButton.setVisibility(View.GONE);
    }

    /**
     * @return The {@link Context} with the modal dialog theme set.
     */
    public Context getContext() {
        return mContext;
    }

    /**
     * @return The content view of this dialog.
     */
    public View getView() {
        return mDialogView;
    }

    /**
     * @return The controller that controls the actions on the dialogs.
     */
    public Controller getController() {
        return mController;
    }

    /**
     * @param title The title of the dialog. If empty string, the title view will not show.
     */
    public void setTitle(String title) {
        mTitleView.setText(title);
    }

    /**
     * @param message The message of the dialog. If empty string, the title view will not show.
     */
    public void setMessage(String message) {
        mMessageView.setText(message);
    }

    /**
     * @param view The customized view of the dialog. If not set, the custom view will not show.
     */
    public void setCustomView(View view) {
        mCustom = view;
    }

    /**
     * @param resId String resource id of the positive button text.
     */
    public void setPositiveButton(@StringRes int resId) {
        mPositiveButton.setText(resId);
        mPositiveButton.setOnClickListener(this);
    }

    /**
     * @param resId String resource id of the negative button text.
     */
    public void setNegativeButton(@StringRes int resId) {
        mNegativeButton.setText(resId);
        mNegativeButton.setOnClickListener(this);
    }
}
