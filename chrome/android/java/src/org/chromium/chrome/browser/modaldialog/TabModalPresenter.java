// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import android.content.res.Resources;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.ViewStub;
import android.widget.FrameLayout;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.compositor.bottombar.OverlayPanel;
import org.chromium.chrome.browser.contextualsearch.ContextualSearchManager;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheetObserver;
import org.chromium.chrome.browser.widget.bottomsheet.EmptyBottomSheetObserver;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content_public.common.BrowserControlsState;

/** The presenter that displays tab modal dialogs. It also manages
 * actions outside a dialog (e.g. browser controls) based on whether a dialog is showing.*/
public class TabModalPresenter extends ModalDialogManager.Presenter {
    /** The observer to dismiss all dialogs when the attached tab is not interactable. */
    private final TabObserver mTabObserver = new EmptyTabObserver() {
        @Override
        public void onInteractabilityChanged(boolean isInteractable) {
            if (!isInteractable) {
                mManager.cancelAllDialogs();
            }
        }
    };

    /** The activity displaying the dialogs. */
    private final ChromeActivity mChromeActivity;

    /** Whether browser controls are at bottom */
    private final boolean mHasBottomControls;

    /**
     * The observer to change view hierarchy for the dialog container when the sheet is opened or
     * closed.
     */
    private BottomSheetObserver mBottomSheetObserver;

    /** The parent view that contains the dialog container. */
    private ViewGroup mContainerParent;

    /** The container view that a dialog to be shown will be attached to. */
    private ViewGroup mDialogContainer;

    /** The content view of the dialog that is currently showing. */
    private View mCurrentContentView;

    /** A dialog is currently being shown. */
    private boolean mIsShowing;

    /** Whether the dialog container is brought to the front in its parent. */
    private boolean mContainerIsAtFront;

    /** Whether the action bar on selected text is temporarily cleared for showing dialogs. */
    private boolean mDidClearTextControls;

    /**
     * The sibling view of the dialog container drawn next in its parent when it should be behind
     * browser controls. If BottomSheet is opened or UrlBar is focused, the dialog container should
     * be behind the browser controls and the URL suggestions.
     */
    private View mDefaultNextSiblingView;

    /**
     * Constructor for initializing dialog container.
     * @param chromeActivity The activity displaying the dialogs.
     */
    public TabModalPresenter(ChromeActivity chromeActivity) {
        mChromeActivity = chromeActivity;
        mHasBottomControls = mChromeActivity.getBottomSheet() != null;

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

    @Override
    public void setDialogView(ModalDialogView dialog) {
        if (dialog == null) {
            dismiss();
        } else {
            show(dialog.getView());
        }
    }

    /**
     * Notified when the focus of the omnibox has changed.
     * @param hasFocus Whether the omnibox currently has focus.
     */
    public void onOmniboxFocusChanged(boolean hasFocus) {
        // If has bottom controls, the view hierarchy will be updated by mBottomSheetObserver.
        if (mIsShowing && !mHasBottomControls) updateContainerHierarchy(!hasFocus);
    }

    /**
     * Show the specified view in the dialog container.
     * @param view The view to be added to the dialog container.
     */
    private void show(View view) {
        mIsShowing = true;
        if (mDialogContainer == null) initDialogContainer();
        setBrowserControlsAccess(true);
        mDialogContainer.setVisibility(View.VISIBLE);

        mCurrentContentView = view;
        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
                MarginLayoutParams.MATCH_PARENT, MarginLayoutParams.WRAP_CONTENT, Gravity.CENTER);
        mDialogContainer.addView(mCurrentContentView, params);
        mChromeActivity.addViewObscuringAllTabs(mCurrentContentView);
    }

    /**
     * Remove the current showing view from the dialog container.
     */
    private void dismiss() {
        mIsShowing = false;
        setBrowserControlsAccess(false);
        mDialogContainer.removeView(mCurrentContentView);
        mDialogContainer.setVisibility(View.INVISIBLE);
        mChromeActivity.removeViewObscuringAllTabs(mCurrentContentView);
        mCurrentContentView = null;
    }

    /**
     * Change view hierarchy for the dialog container to be either the front most or beneath the
     * toolbar.
     * @param toFront Whether the dialog container should be brought to the front.
     */
    private void updateContainerHierarchy(boolean toFront) {
        if (toFront == mContainerIsAtFront) return;
        mContainerIsAtFront = toFront;
        if (toFront) {
            mDialogContainer.bringToFront();
        } else {
            mContainerParent.removeView(mDialogContainer);
            mContainerParent.addView(
                    mDialogContainer, mContainerParent.indexOfChild(mDefaultNextSiblingView));
        }
    }

    /**
     * Inflate the dialog container in the dialog container view stub.
     */
    private void initDialogContainer() {
        ViewStub dialogContainerStub =
                mChromeActivity.findViewById(R.id.tab_modal_dialog_container_stub);
        dialogContainerStub.setLayoutResource(R.layout.modal_dialog_container);

        mDialogContainer = (ViewGroup) dialogContainerStub.inflate();
        mContainerParent = (ViewGroup) mDialogContainer.getParent();
        // The default sibling view is the next view of the dialog container stub in main.xml and
        // should not be removed from its parent.
        mDefaultNextSiblingView = mChromeActivity.findViewById(R.id.fading_focus_target);

        // Set the margin of the container and the dialog scrim so that the scrim doesn't overlap
        // the toolbar.
        Resources resources = mChromeActivity.getResources();
        int scrimVerticalMargin =
                resources.getDimensionPixelSize(R.dimen.tab_modal_scrim_vertical_margin);
        int containerVerticalMargin =
                resources.getDimensionPixelSize(mChromeActivity.getControlContainerHeightResource())
                - scrimVerticalMargin;

        MarginLayoutParams params = (MarginLayoutParams) mDialogContainer.getLayoutParams();
        params.width = ViewGroup.MarginLayoutParams.MATCH_PARENT;
        params.height = ViewGroup.MarginLayoutParams.MATCH_PARENT;
        params.topMargin = !mHasBottomControls ? containerVerticalMargin : 0;
        params.bottomMargin = mHasBottomControls ? containerVerticalMargin : 0;
        mDialogContainer.setLayoutParams(params);

        View scrimView = mDialogContainer.findViewById(R.id.scrim);
        params = (MarginLayoutParams) scrimView.getLayoutParams();
        params.width = MarginLayoutParams.MATCH_PARENT;
        params.height = MarginLayoutParams.MATCH_PARENT;
        params.topMargin = !mHasBottomControls ? scrimVerticalMargin : 0;
        params.bottomMargin = mHasBottomControls ? scrimVerticalMargin : 0;
        scrimView.setLayoutParams(params);
    }

    /**
     * Set whether the browser controls access should be restricted. If true, dialogs are expected
     * to be showing and overflow menu would be disabled.
     * @param restricted Whether the browser controls access should be restricted.
     */
    private void setBrowserControlsAccess(boolean restricted) {
        BottomSheet bottomSheet = mChromeActivity.getBottomSheet();
        View menuButton = mChromeActivity.getToolbarManager().getMenuButton();
        Tab activeTab = mChromeActivity.getActivityTab();
        ContentViewCore contentViewCore = activeTab != null ? activeTab.getContentViewCore() : null;

        if (restricted) {
            // Hide contextual search panel so that bottom toolbar will not be
            // obscured and back press is not overwritten.
            ContextualSearchManager contextualSearchManager =
                    mChromeActivity.getContextualSearchManager();
            if (contextualSearchManager != null) {
                contextualSearchManager.hideContextualSearch(
                        OverlayPanel.StateChangeReason.UNKNOWN);
            }

            // Dismiss the action bar that obscures the dialogs but preserve the text selection.
            if (contentViewCore != null) {
                contentViewCore.preserveSelectionOnNextLossOfFocus();
                contentViewCore.getContainerView().clearFocus();
                contentViewCore.updateTextSelectionUI(false);
                mDidClearTextControls = true;
            }

            // Force toolbar to show and disable overflow menu.
            // TODO(huayinz): figure out a way to avoid |UpdateBrowserControlsState| being blocked
            // by render process stalled due to javascript dialog.
            if (activeTab != null) {
                mChromeActivity.getFullscreenManager().setPositionsForTabToNonFullscreen();
                activeTab.updateBrowserControlsState(BrowserControlsState.SHOWN, false);
            }

            if (mHasBottomControls) {
                bottomSheet.setSheetState(BottomSheet.SHEET_STATE_PEEK, true);
                bottomSheet.addObserver(mBottomSheetObserver);
            } else {
                mChromeActivity.getToolbarManager().setUrlBarFocus(false);
            }
            menuButton.setEnabled(false);

            // Add Observers to handle dialog dismissal.
            activeTab.addObserver(mTabObserver);
            updateContainerHierarchy(true);
        } else {
            // Show the action bar back if it was dismissed when the dialogs were showing.
            if (mDidClearTextControls) {
                mDidClearTextControls = false;
                if (contentViewCore != null) {
                    contentViewCore.updateTextSelectionUI(true);
                }
            }
            menuButton.setEnabled(true);

            activeTab.removeObserver(mTabObserver);
            if (mHasBottomControls) bottomSheet.removeObserver(mBottomSheetObserver);
        }
    }

    @VisibleForTesting
    View getDialogContainerForTest() {
        return mDialogContainer;
    }

    @VisibleForTesting
    ViewGroup getContainerParentForTest() {
        return mContainerParent;
    }
}
