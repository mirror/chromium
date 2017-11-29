// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.util.SparseArray;

import org.chromium.base.VisibleForTesting;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import javax.annotation.Nonnull;

/**
 * Manager for managing the display of a queue of {@link ModalDialogView}s.
 */
public class ModalDialogManager {
    /**
     * Present a {@link ModalDialogView} in a container.
     */
    public static abstract class Presenter {
        protected ModalDialogManager mManager;

        /**
         * @param manager The manager that this presenter is registered to.
         */
        private void setManager(ModalDialogManager manager) {
            mManager = manager;
        }

        /**
         * Shows the specified {@link ModalDialogView}. If the specified {@link ModalDialogView} is
         * null, dismiss the dialog view that is currently showing.
         * @param dialog The {@link ModalDialogView} that needs to be shown, or null to reset.
         */
        public abstract void setDialogView(@Nullable ModalDialogView dialog);
    }

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({APP_MODAL, TAB_MODAL})
    public @interface ModalDialogType {}
    public static final int APP_MODAL = 0;
    public static final int TAB_MODAL = 1;

    /** Mapping of the {@link Presenter}s and the type of dialogs they are showing. */
    private final SparseArray<Presenter> mPresenters = new SparseArray<>();

    /** Mapping of the pending dialogs and the their type. */
    private final Map<ModalDialogView, Integer> mPendingDialogTypes = new HashMap<>();

    /** The list of pending dialogs */
    private final List<ModalDialogView> mPendingDialogs = new LinkedList<>();

    /** The default presenter to be used if a specified type is not supported. */
    private final Presenter mDefaultPresenter;

    /** The dialog that is currently showing and its type. */
    private ModalDialogView mCurrentDialog;

    /** The type of the dialog that is currently showing. */
    private Presenter mCurrentPresenter;

    public ModalDialogManager(
            @Nonnull Presenter defaultPresenter, @ModalDialogType int defaultType) {
        mDefaultPresenter = defaultPresenter;
        registerPresenter(defaultPresenter, defaultType);
    }

    /**
     * Register a {@link Presenter} that shows a specific type of dialogs. Note that only one
     * presenter of each type can be registered.
     * @param presenter The {@link Presenter} to be registered.
     * @param dialogType The type of the dialog shown by the specified presenter.
     */
    public void registerPresenter(Presenter presenter, @ModalDialogType int dialogType) {
        assert mPresenters.get(dialogType)
                == null : "Only one presenter can be registered for each type.";
        mPresenters.put(dialogType, presenter);
        presenter.setManager(this);
    }

    /**
     * @return Whether a dialog is currently showing.
     */
    public boolean isShowing() {
        return mCurrentDialog != null;
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
        mCurrentPresenter = mPresenters.get(dialogType, mDefaultPresenter);
        mCurrentPresenter.setDialogView(dialog);
    }

    /**
     * Dismiss the specified dialog. If the dialog is not currently showing, it will be removed from
     * the pending dialog list.
     * @param dialog The dialog to be dismissed or removed from pending list.
     */
    public void dismissDialog(ModalDialogView dialog) {
        if (dialog != mCurrentDialog) {
            mPendingDialogTypes.remove(dialog);
            mPendingDialogs.remove(dialog);
            return;
        }
        if (!isShowing()) return;

        mCurrentPresenter.setDialogView(null);
        mCurrentDialog = null;
        mCurrentPresenter = null;

        if (!mPendingDialogs.isEmpty()) {
            ModalDialogView nextDialog = mPendingDialogs.remove(0);
            showDialog(nextDialog, mPendingDialogTypes.remove(nextDialog));
        }
    }

    /**
     * Cancel showing the specified dialog. This is essentially the same as
     * {@link #dismissDialog(ModalDialogView)} but will also call the onCancelled callback from the
     * modal dialog.
     * @param dialog The dialog to be cancelled.
     */
    public void cancelDialog(ModalDialogView dialog) {
        dismissDialog(dialog);
        dialog.getController().onCancel();
    }

    /**
     * Handle a back press event.
     */
    public boolean handleBackPress() {
        if (!isShowing()) return false;
        cancelDialog(mCurrentDialog);
        return true;
    }

    /**
     * Dismiss the dialog currently shown and remove all pending dialogs and call the onCancelled
     * callbacks from the modal dialogs.
     */
    protected void cancelAllDialogs() {
        while (!mPendingDialogs.isEmpty()) {
            mPendingDialogs.remove(0).getController().onCancel();
        }
        cancelDialog(mCurrentDialog);
    }

    @VisibleForTesting
    List getPendingDialogsForTest() {
        return mPendingDialogs;
    }

    Presenter getPresenterForTest(@ModalDialogType int dialogType) {
        return mPresenters.get(dialogType);
    }
}
