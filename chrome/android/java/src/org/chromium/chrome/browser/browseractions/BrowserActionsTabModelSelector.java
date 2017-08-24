// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.app.Activity;

import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.browseractions.BrowserActionsTabCreatorManager.BrowserActionsTabCreator;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelObserver;
import org.chromium.chrome.browser.tabmodel.IncognitoTabModel;
import org.chromium.chrome.browser.tabmodel.IncognitoTabModelImplCreator;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModel.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModelDelegate;
import org.chromium.chrome.browser.tabmodel.TabModelImpl;
import org.chromium.chrome.browser.tabmodel.TabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelOrderController;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorBase;
import org.chromium.chrome.browser.tabmodel.TabPersistentStore;
import org.chromium.chrome.browser.tabmodel.TabPersistentStore.TabPersistentStoreObserver;
import org.chromium.content_public.browser.LoadUrlParams;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * The tab model selector creates Tabs for Browser Actions. Tabs created by it are not shown in the
 * foreground and don't need {@link TabContentMananger}.
 */
public class BrowserActionsTabModelSelector
        extends TabModelSelectorBase implements TabModelDelegate {
    /** The singleton reference. */
    private static BrowserActionsTabModelSelector sInstance;

    private final TabCreatorManager mTabCreatorManager;

    private final TabPersistentStore mTabSaver;

    private final TabModelOrderController mOrderController;

    /** Flag set to false when the asynchronous loading of tabs is finished. */
    private final AtomicBoolean mSessionRestoreInProgress = new AtomicBoolean(true);

    /** This flag signifies the object has gotten an onNativeReady callback and
    /* has not been destroyed. */
    private boolean mActiveState;

    /**
     * Builds a {@link BrowserActionsTabModelSelector} instance.
     *
     * @param activity An {@link Activity} instance.
     * @param tabCreatorManager A {@link TabCreatorManager} instance.
     */
    private BrowserActionsTabModelSelector(Activity activity, TabCreatorManager tabCreatorManager) {
        super();
        mTabCreatorManager = tabCreatorManager;
        final TabPersistentStoreObserver persistentStoreObserver =
                new TabPersistentStoreObserver() {
                    @Override
                    public void onStateLoaded() {
                        markTabStateInitialized();
                    }
                };
        BrowserActionsTabPersistencePolicy persistencePolicy =
                new BrowserActionsTabPersistencePolicy();
        mTabSaver = new TabPersistentStore(
                persistencePolicy, this, mTabCreatorManager, persistentStoreObserver);
        mOrderController = new TabModelOrderController(this);
    }

    /**
     * @return The singleton instance of {@link BrowserActionsTabModelSelector}.
     */
    public static BrowserActionsTabModelSelector getInstance(
            Activity activity, TabCreatorManager tabCreatorManager) {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new BrowserActionsTabModelSelector(activity, tabCreatorManager);
        }
        return sInstance;
    }

    @Override
    public void markTabStateInitialized() {
        super.markTabStateInitialized();
        if (!mSessionRestoreInProgress.getAndSet(false)) return;

        // This is the first time we set
        // |mSessionRestoreInProgress|, so we need to broadcast.
        TabModelImpl model = getModel();

        if (model != null) {
            model.broadcastSessionRestoreComplete();
        } else {
            assert false : "Normal tab model is null after tab state loaded.";
        }
    }

    /**
     * Initializes the selectors for the {@link BrowserActionsTabModelSelector}.
     */
    public void initializeSelector() {
        assert !mActiveState : "onNativeLibraryReady called twice!";
        BrowserActionsTabCreator regularTabCreator =
                (BrowserActionsTabCreator) mTabCreatorManager.getTabCreator(false);
        BrowserActionsTabCreator incognitoTabCreator =
                (BrowserActionsTabCreator) mTabCreatorManager.getTabCreator(true);
        TabModelImpl normalModel = new TabModelImpl(false, false, regularTabCreator,
                incognitoTabCreator, null, mOrderController, null, mTabSaver, this, false);
        TabModel incognitoModel =
                new IncognitoTabModel(new IncognitoTabModelImplCreator(regularTabCreator,
                        incognitoTabCreator, null, mOrderController, null, mTabSaver, this));
        initialize(isIncognitoSelected(), normalModel, incognitoModel);
        mActiveState = true;
        TabModelObserver tabModelObserver = new EmptyTabModelObserver() {
            @Override
            public void didAddTab(Tab tab, TabLaunchType type) {
                if (type == TabLaunchType.FROM_BROWSER_ACTIONS) {
                    notifyNewTabCreated(tab);
                }
            }
        };
        normalModel.addObserver(tabModelObserver);
    }

    /**
     * @return Whether the selector has been initialized.
     */
    public boolean isActiveState() {
        return mActiveState;
    }

    private void notifyNewTabCreated(Tab tab) {
        if (tab != null) mTabSaver.addTabToSaveQueue(tab);
        BrowserActionsService.sendIntent(
                BrowserActionsService.ACTION_TAB_CREATION_FINISH, Tab.INVALID_TAB_ID);
    }

    @Override
    public Tab openNewTab(
            LoadUrlParams loadUrlParams, TabLaunchType type, Tab parent, boolean incognito) {
        return mTabCreatorManager.getTabCreator(incognito).createNewTab(
                loadUrlParams, type, parent);
    }

    @Override
    public void requestToShowTab(Tab tab, TabSelectionType type) {}

    @Override
    public boolean closeAllTabsRequest(boolean incognito) {
        return false;
    }

    @Override
    public boolean isInOverviewMode() {
        return false;
    }

    @Override
    public boolean isSessionRestoreInProgress() {
        return mSessionRestoreInProgress.get();
    }

    public TabModelImpl getModel() {
        return (TabModelImpl) getModel(false);
    }

    public void saveState() {
        commitAllTabClosures();
        mTabSaver.saveState();
    }

    /**
     * Load the saved tab state. This should be called before any new tabs are created. The saved
     * tabs shall not be restored until {@link #restoreTabs} is called.
     * @param ignoreIncognitoFiles Whether to skip loading incognito tabs.
     */
    public void loadState(boolean ignoreIncognitoFiles) {
        mTabSaver.loadState(ignoreIncognitoFiles);
    }

    /**
     * Restore the saved tabs which were loaded by {@link #loadState}.
     *
     * @param setActiveTab If true, synchronously load saved active tab and set it as the current
     *                     active tab.
     */
    public void restoreTabs(boolean setActiveTab) {
        mTabSaver.restoreTabs(setActiveTab);
    }

    @Override
    @VisibleForTesting
    protected void clearData() {
        mActiveState = false;
        sInstance = null;
        super.clearData();
    }
}
