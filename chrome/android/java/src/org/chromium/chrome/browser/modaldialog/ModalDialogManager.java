// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import android.app.Activity;
import android.app.Dialog;
import android.support.annotation.IntDef;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.AlwaysDismissedDialog;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

/**
 * Manager for managing the display of a queue of {@link ModalDialogView}s.
 */
public class ModalDialogManager {
    /**
     * Present a {@link ModalDialogView} in a container.
     */
    public interface Presenter {
        /**
         * Shows the specified {@link ModalDialogView}.
         * @param dialog The {@link ModalDialogView} that needs to be shown.
         */
        void showDialog(ModalDialogView dialog);

        /**
         * Dismiss the display of {@link ModalDialogView} that is currently showing.
         */
        void dismissDialog();
    }

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({APP_MODAL, TAB_MODAL})
    public @interface ModalDialogType {}
    public static final int APP_MODAL = 0;
    public static final int TAB_MODAL = 1;

    /** Mapping of the pending dialogs and the their type. */
    private final Map<ModalDialogView, Integer> mPendingDialogTypes = new HashMap<>();

    /** The list of pending dialogs */
    private final List<ModalDialogView> mPendingDialogs = new LinkedList<>();

    /** The activity displaying the dialogs. */
    private final Activity mActivity;

    /** The presenter that shows a {@link ModalDialogView} in an Android dialog. */
    private final Presenter mAppModalPresenter = new Presenter() {
        Dialog mDialog;
        ViewGroup mContainer;
        View mDialogView;

        @Override
        public void showDialog(ModalDialogView dialog) {
            if (mDialog == null) {
                mDialog = new AlwaysDismissedDialog(mActivity, R.style.ModalDialogTheme);
                mDialog.setOnCancelListener(dialogInterface -> handleBackPress());
                mContainer = (ViewGroup) LayoutInflater.from(mActivity).inflate(
                        R.layout.modal_dialog_container, null);
                mDialog.setContentView(mContainer);
            }
            mDialogView = dialog.getView();
            FrameLayout.LayoutParams params =
                    new FrameLayout.LayoutParams(ViewGroup.MarginLayoutParams.MATCH_PARENT,
                            ViewGroup.MarginLayoutParams.WRAP_CONTENT, Gravity.CENTER);
            mContainer.addView(mDialogView, params);
            mDialog.show();
        }

        @Override
        public void dismissDialog() {
            if (mDialog == null) return;
            mDialog.dismiss();
            mContainer.removeView(mDialogView);
            mDialogView = null;
        }
    };

    /** The dialog that is currently showing and its type. */
    private ModalDialogView mCurrentDialog;

    public ModalDialogManager(Activity activity) {
        mActivity = activity;
    }

    /**
     * @return Whether a dialog is currently showing.
     */
    public boolean isShowing() {
        return mCurrentDialog != null;
    }

    /**
     * @return If a dialog is showing, return the {@link Presenter} that shows app modal dialogs
     *         regardless of the type of the dialog. If no dialog is showing, return null.
     */
    @Nullable
    protected Presenter getCurrentPresenter() {
        return isShowing() ? mAppModalPresenter : null;
    }

    /**
     * Show the specified dialog. If another dialog is currently showing, the specified dialog will
     * be added to the pending dialog list.
     * @param dialog The dialog to be shown or added to pending list.
     * @param dialogType The type of the dialog to be shown.
     */
    public void showDialog(ModalDialogView dialog, @ModalDialogType int dialogType) {
        if (isShowing()) {
            mPendingDialogTypes.put(dialog, dialogType);
            mPendingDialogs.add(dialog);
            return;
        }

        mCurrentDialog = dialog;
        mCurrentDialog.prepareBeforeShow();
        getCurrentPresenter().showDialog(dialog);
    }

    /**
     * Dismiss the specified dialog. If the dialog is not currently showing, it will be removed from
     * the pending dialog list.
     * @param dialog The dialog to be dismissed or removed from pending list.
     */
    public void dismissDialog(ModalDialogView dialog) {
        if (!isShowing()) return;
        if (dialog != mCurrentDialog) {
            mPendingDialogTypes.remove(dialog);
            mPendingDialogs.remove(dialog);
            return;
        }

        getCurrentPresenter().dismissDialog();
        mCurrentDialog = null;

        if (!mPendingDialogs.isEmpty()) {
            ModalDialogView nextDialog = mPendingDialogs.remove(0);
            showDialog(nextDialog, mPendingDialogTypes.remove(nextDialog));
        }
    }

    /**
     * Handle a back press event.
     */
    public boolean handleBackPress() {
        if (!isShowing()) return false;

        ModalDialogView dialog = mCurrentDialog;
        dialog.getController().onDismissNoAction();
        dismissDialog(dialog);
        return true;
    }

    /**
     * Dismiss the dialog currently shown and remove all pending dialogs.
     */
    protected void dismissAllDialogs() {
        while (!mPendingDialogs.isEmpty()) {
            mPendingDialogs.remove(0).getController().onDismissNoAction();
        }

        // Dismiss the dialog that is currently showing.
        handleBackPress();
    }

    @VisibleForTesting
    List getPendingDialogsForTest() {
        return mPendingDialogs;
    }
}
