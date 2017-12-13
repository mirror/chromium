// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabModelObserver;

/**
 * Class responsible for handling dismissal of a tab modal dialog on user actions outside the tab
 * modal dialog.
 */
public class TabModalLifetimeHandler {
    /** The observer to dismiss all dialogs when the attached tab is not interactable. */
    private final TabObserver mTabObserver = new EmptyTabObserver() {
        @Override
        public void onInteractabilityChanged(boolean isInteractable) {
            if (!isInteractable && mPresenter.getModalDialog() != null) {
                mManager.cancelAllDialogs();
            }
        }
    };

    private final ModalDialogManager mManager;
    private final TabModalPresenter mPresenter;
    private final TabModelSelectorTabModelObserver mTabModelObserver;

    private Tab mActiveTab;

    /**
     * @param activity The {@link ChromeActivity} that this handler is attached to.
     * @param manager The {@link ModalDialogManager} that this handler handles.
     */
    public TabModalLifetimeHandler(ChromeActivity activity, ModalDialogManager manager) {
        mManager = manager;
        mPresenter = new TabModalPresenter(activity);
        mManager.registerPresenter(mPresenter, ModalDialogManager.TAB_MODAL);

        TabModelSelector tabModelSelector = activity.getTabModelSelector();
        mTabModelObserver = new TabModelSelectorTabModelObserver(activity.getTabModelSelector()) {
            @Override
            public void didSelectTab(Tab tab, TabModel.TabSelectionType type, int lastId) {
                if (mActiveTab != null) mActiveTab.removeObserver(mTabObserver);
                mActiveTab = tabModelSelector.getCurrentTab();
                if (mActiveTab != null) mActiveTab.addObserver(mTabObserver);
            }
        };
    }

    public TabModalPresenter getPresenter() {
        return mPresenter;
    }

    public void destroy() {
        mTabModelObserver.destroy();
    }
}
