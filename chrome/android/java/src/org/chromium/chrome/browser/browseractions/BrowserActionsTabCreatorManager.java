// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.content.Context;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.TabState;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabDelegateFactory;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/**
 * The manager returns a {@link BrowserActionsTabCreator} to create Tabs for Browser Actions.
 */
public class BrowserActionsTabCreatorManager implements TabCreatorManager {
    private final BrowserActionsTabCreator mTabCreator;

    public BrowserActionsTabCreatorManager() {
        mTabCreator = new BrowserActionsTabCreator();
    }

    /**
     * This class creates various kinds of new tabs specific for Browser Actions.
     * The created tabs are not bound with {@link ChromeActivity}.
     */
    public class BrowserActionsTabCreator extends TabCreator {
        private TabModel mTabModel;

        @Override
        public boolean createsTabsAsynchronously() {
            return true;
        }

        @Override
        public Tab createNewTab(LoadUrlParams loadUrlParams, TabLaunchType type, Tab parent) {
            Context context = ContextUtils.getApplicationContext();
            WindowAndroid windowAndroid = new WindowAndroid(context);
            Tab tab = Tab.createTabForLazyLoad(
                    false, windowAndroid, type, Tab.INVALID_TAB_ID, loadUrlParams);
            tab.initialize(null, null, new TabDelegateFactory(), true, false);
            mTabModel.addTab(tab, -1, TabLaunchType.FROM_BROWSER_ACTIONS);
            return tab;
        }

        @Override
        public Tab createFrozenTab(TabState state, int id, int index) {
            Tab tab = Tab.createFrozenTabFromState(id, false, null, Tab.INVALID_TAB_ID, state);
            mTabModel.addTab(tab, index, TabLaunchType.FROM_RESTORE);
            return tab;
        }

        @Override
        public Tab launchUrl(String url, TabLaunchType type) {
            return createNewTab(new LoadUrlParams(url), type, null);
        }

        @Override
        public boolean createTabWithWebContents(
                Tab parent, WebContents webContents, int parentId, TabLaunchType type, String url) {
            createNewTab(new LoadUrlParams(url), type, null);
            return true;
        }

        /**
         * Sets the tab model and tab content manager to use.
         * @param model The new {@link TabModel} to use.
         */
        public void setTabModel(TabModel model) {
            mTabModel = model;
        }
    }

    @Override
    public TabCreator getTabCreator(boolean incognito) {
        return mTabCreator;
    }
}
