// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import android.content.res.Resources;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.compositor.bottombar.OverlayPanel;
import org.chromium.chrome.browser.compositor.layouts.EmptyOverviewModeObserver;
import org.chromium.chrome.browser.compositor.layouts.OverviewModeBehavior.OverviewModeObserver;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheetObserver;
import org.chromium.chrome.browser.widget.bottomsheet.EmptyBottomSheetObserver;
import org.chromium.content.browser.ContentViewCore;

import java.util.LinkedList;
import java.util.List;

/**
 * Manager for the tab modal dialogs showing in {@link ChromeTabbedActivity}. It manages a list of
 * dialogs so that only one dialog may be shown at all time. It also manages actions outside a
 * dialog (e.g. browser controls) based on whether a dialog is showing.
 */
public class TabModalDialogManager {
    /** The observer to dismiss all dialogs when tab switcher is shown. */
    private final OverviewModeObserver mOverviewModeObserver = new EmptyOverviewModeObserver() {
        @Override
        public void onOverviewModeStartedShowing(boolean showToolbar) {
            dismissAllDialogs();
        }
    };

    /** The observer to dismiss all dialogs when another tab is selected to be shown. */
    private final TabModelObserver mTabModelObserver = new EmptyTabModelObserver() {
        @Override
        public void didSelectTab(Tab tab, TabModel.TabSelectionType type, int lastId) {
            if (tab.getId() != lastId) {
                dismissAllDialogs();
            }
        }
    };

    /** The observer to change view hierarchy for dialog container on sheet is opened or closed. */
    private BottomSheetObserver mBottomSheetObserver;

    /** The list of the pending dialogs. */
    private final List<ModalDialog> mPendingDialogs = new LinkedList<>();

    /** The activity displaying the dialogs. */
    private ChromeTabbedActivity mChromeTabbedActivity;

    /** The parent view that contains dialog container. */
    private ViewGroup mContainerParent;

    /** The container view that a dialog to be shown will be attached to. */
    private ViewGroup mDialogContainer;

    /** The dialog that is currently showing. */
    private ModalDialog mCurrentDialog;

    /** Whether browser controls are at bottom */
    private boolean mHasBottomControls;

    /** Whether browser controls access is restricted. */
    private boolean mIsRestrictedBrowserControls;

    /** The index of dialog container in its parent when it should be behind browser controls */
    private int mDefaultContainerIndexInParent;

    /**
     * Constructor for initializing dialog container.
     * @param chromeTabbedActivity The activity displaying the dialogs.
     */
    public TabModalDialogManager(ChromeTabbedActivity chromeTabbedActivity) {
        mChromeTabbedActivity = chromeTabbedActivity;
        mDialogContainer = chromeTabbedActivity.findViewById(R.id.tab_modal_dialog_container);
        mHasBottomControls = mChromeTabbedActivity.getBottomSheet() != null;

        Resources resources = mChromeTabbedActivity.getResources();
        int scrimVerticalMargin =
                resources.getDimensionPixelSize(R.dimen.tab_modal_scrim_vertical_margin);
        int containerVerticalMargin =
                resources.getDimensionPixelSize(
                        mChromeTabbedActivity.getControlContainerHeightResource())
                - scrimVerticalMargin;
        MarginLayoutParams params = (MarginLayoutParams) mDialogContainer.getLayoutParams();

        params.width = ViewGroup.MarginLayoutParams.MATCH_PARENT;
        params.height = ViewGroup.MarginLayoutParams.MATCH_PARENT;
        params.topMargin = !mHasBottomControls ? containerVerticalMargin : 0;
        params.bottomMargin = mHasBottomControls ? containerVerticalMargin : 0;
        mDialogContainer.setLayoutParams(params);

        mContainerParent = (ViewGroup) mDialogContainer.getParent();
        mDefaultContainerIndexInParent = mContainerParent.indexOfChild(mDialogContainer);

        if (mHasBottomControls) {
            mBottomSheetObserver = new EmptyBottomSheetObserver() {
                @Override
                public void onSheetOpened(@BottomSheet.StateChangeReason int reason) {
                    updateContainerHierarchy(false);
                }

                @Override
                public void onSheetClosed(@BottomSheet.StateChangeReason int reason) {
                    super.onSheetClosed(reason);
                    updateContainerHierarchy(true);
                }
            };
        }
    }

    /**
     * @return Whether a dialog is currently showing.
     */
    public boolean isShowing() {
        return mCurrentDialog != null;
    }

    /**
     * Handle a back press event.
     */
    public void handleBackPress() {
        mCurrentDialog.dismissNoAction();
    }

    /**
     * Change view hierarchy for the dialog container to be either the front most or beneath the
     * toolbar.
     * @param toFront Whether the dialog container should be bring to front.
     */
    private void updateContainerHierarchy(boolean toFront) {
        if (toFront) {
            mDialogContainer.bringToFront();
        } else {
            mContainerParent.removeView(mDialogContainer);
            mContainerParent.addView(mDialogContainer, mDefaultContainerIndexInParent);
        }
    }

    /**
     * Notified when the focus of the omnibox has changed.
     * @param hasFocus Whether the omnibox currently has focus.
     */
    public void onOmniboxFocusChanged(boolean hasFocus) {
        // If has bottom controls, the view hierarchy will be updated by mBottomSheetObserver.
        if (isShowing() && !mHasBottomControls) updateContainerHierarchy(!hasFocus);
    }

    /**
     * Dismiss the dialog currently shown and remove all pending dialogs.
     */
    private void dismissAllDialogs() {
        while (!mPendingDialogs.isEmpty()) {
            mPendingDialogs.remove(0).dismissNoAction();
        }
        mCurrentDialog.dismissNoAction();
    }

    /**
     * Show the specified dialog. If another dialog is currently showing, the specified dialog will
     * be added to the pending dialog list.
     * @param dialog The dialog to be shown or added to pending list.
     */
    public void showDialog(ModalDialog dialog) {
        if (isShowing()) {
            mPendingDialogs.add(dialog);
            return;
        }

        if (!mIsRestrictedBrowserControls) {
            setBrowserControlsAccess(true);
            mDialogContainer.setVisibility(View.VISIBLE);
        }
        mCurrentDialog = dialog;

        View dialogView = mCurrentDialog.getDialogView();
        mDialogContainer.addView(dialogView);
        mChromeTabbedActivity.addViewObscuringAllTabs(dialogView);
    }

    /**
     * Dismiss the specified dialog. If the dialog is not currently showing, it will be removed from
     * the pending dialog list.
     * @param dialog The dialog to be dismissed or removed from pending list.
     */
    public void dismissDialog(ModalDialog dialog) {
        if (!isShowing()) return;

        if (dialog != mCurrentDialog) {
            mPendingDialogs.remove(dialog);
            return;
        }

        View dialogView = mCurrentDialog.getDialogView();
        mDialogContainer.removeView(dialogView);
        mChromeTabbedActivity.removeViewObscuringAllTabs(dialogView);

        mCurrentDialog = null;

        if (mPendingDialogs.isEmpty() && mIsRestrictedBrowserControls) {
            setBrowserControlsAccess(false);
            mDialogContainer.setVisibility(View.GONE);
        } else {
            showDialog(mPendingDialogs.remove(0));
        }
    }

    /**
     * Set whether the browser controls access should be restricted. If true, dialogs are expected
     * to be showing and overflow menu would be disabled.
     * @param restricted Whether the browser controls access should be restricted.
     */
    private void setBrowserControlsAccess(boolean restricted) {
        mIsRestrictedBrowserControls = restricted;

        BottomSheet bottomSheet = mChromeTabbedActivity.getBottomSheet();
        View menuButton = mChromeTabbedActivity.getToolbarManager().getMenuButton();
        TabModelSelector selector = mChromeTabbedActivity.getTabModelSelector();
        if (mIsRestrictedBrowserControls) {
            // Hide contextual search panel so that bottom toolbar will not be
            // obscured and back press is not overwritten.
            ContextualSearchManager contextualSearchManager =
                    mChromeTabbedActivity.getContextualSearchManager();
            if (contextualSearchManager != null) {
                contextualSearchManager.hideContextualSearch(
                        OverlayPanel.StateChangeReason.UNKNOWN);
            }

            // Destroy action mode to avoid obscuring the dialog. Note that the render process is
            // blocked when JavaScript dialog is showing so the text selection is only cleared after
            // the dialog is dismissed.
            Tab activeTab = mChromeTabbedActivity.getActivityTab();
            if (activeTab != null) {
                ContentViewCore contentViewCore = activeTab.getContentViewCore();
                if (contentViewCore != null) contentViewCore.destroySelectActionMode();
            }

            // Force toolbar to show and disable overflow menu.
            // TODO(huayinz): figure out a way to avoid |UpdateBrowserControlsState| being blocked
            // by render process stalled due to javascript dialog.
            if (mHasBottomControls) {
                bottomSheet.setSheetState(BottomSheet.SHEET_STATE_PEEK, true);
                bottomSheet.addObserver(mBottomSheetObserver);
            }
            menuButton.setEnabled(false);

            // Add Observers to handle dialog dismissal.
            mChromeTabbedActivity.getLayoutManager().addOverviewModeObserver(mOverviewModeObserver);
            for (TabModel tabModel : selector.getModels()) {
                tabModel.addObserver(mTabModelObserver);
            }

            // Bring dialog container to front so that it may overlay the top 8dp of the toolbar.
            if (mContainerParent.indexOfChild(mDialogContainer) == mDefaultContainerIndexInParent) {
                updateContainerHierarchy(true);
            }
        } else {
            // TODO(huayinz): reset browser controls state
            menuButton.setEnabled(true);

            mChromeTabbedActivity.getLayoutManager().removeOverviewModeObserver(
                    mOverviewModeObserver);
            if (selector != null) {
                for (TabModel tabModel : selector.getModels()) {
                    tabModel.removeObserver(mTabModelObserver);
                }
            }
            if (mHasBottomControls) bottomSheet.removeObserver(mBottomSheetObserver);
        }
    }
}
