// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browseractions;

import android.app.Activity;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.provider.Browser;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.browseractions.BrowserActionsTabCreatorManager.BrowserActionsTabCreator;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.share.ShareHelper;
import org.chromium.chrome.browser.share.ShareParams;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.common.Referrer;
import org.chromium.ui.widget.Toast;

import java.lang.ref.WeakReference;

/**
 * A delegate responsible for taking actions based on browser action context menu selections.
 */
public class BrowserActionsContextMenuItemDelegate {
    private static final String TAG = "BrowserActionsItem";

    private final Activity mActivity;
    private final String mSourcePackageName;
    private final BrowserActionsTabCreatorManager mTabCreatorManager;

    /**
     * Builds a {@link BrowserActionsContextMenuItemDelegate} instance.
     * @param activity The activity displays the context menu.
     * @param sourcePackageName The package name of the app which requests the Browser Actions.
     */
    public BrowserActionsContextMenuItemDelegate(Activity activity, String sourcePackageName) {
        mActivity = activity;
        mSourcePackageName = sourcePackageName;
        mTabCreatorManager = new BrowserActionsTabCreatorManager();
    }

    private int openTabInBackground(String linkUrl) {
        Referrer referrer = IntentHandler.constructValidReferrerForAuthority(mSourcePackageName);
        LoadUrlParams loadUrlParams = new LoadUrlParams(linkUrl);
        loadUrlParams.setReferrer(referrer);
        for (WeakReference<Activity> ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref.get() instanceof ChromeTabbedActivity)) continue;

            ChromeTabbedActivity activity = (ChromeTabbedActivity) ref.get();
            if (activity == null) continue;
            if (activity.getTabModelSelector() != null) {
                Tab tab = activity.getTabModelSelector().openNewTab(
                        loadUrlParams, TabLaunchType.FROM_BROWSER_ACTIONS, null, false);
                assert tab != null;
                return tab.getId();
            }
        }
        BrowserActionsTabModelSelector selector =
                BrowserActionsTabModelSelector.getInstance(mActivity, mTabCreatorManager);
        if (!selector.isActiveState()) {
            selector.initializeSelector();
            ((BrowserActionsTabCreator) mTabCreatorManager.getTabCreator(false))
                    .setTabModel(selector.getCurrentModel());
            selector.loadState(true);
            selector.restoreTabs(false);
        }
        Tab tab =
                selector.openNewTab(loadUrlParams, TabLaunchType.FROM_BROWSER_ACTIONS, null, false);
        if (tab != null) return tab.getId();
        return Tab.INVALID_TAB_ID;
    }

    @VisibleForTesting
    BrowserActionsTabCreatorManager getTabCreatorManager() {
        return mTabCreatorManager;
    }

    /**
     * Called when the {@code text} should be saved to the clipboard.
     * @param text The text to save to the clipboard.
     */
    public void onSaveToClipboard(String text) {
        ClipboardManager clipboardManager =
                (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData data = ClipData.newPlainText("url", text);
        clipboardManager.setPrimaryClip(data);
    }

    /**
     * Called when the {@code linkUrl} should be opened in Chrome incognito tab.
     * @param linkUrl The url to open.
     */
    public void onOpenInIncognitoTab(String linkUrl) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(linkUrl));
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setPackage(mActivity.getPackageName());
        intent.putExtra(ChromeLauncherActivity.EXTRA_IS_ALLOWED_TO_RETURN_TO_PARENT, false);
        intent.putExtra(IntentHandler.EXTRA_OPEN_NEW_INCOGNITO_TAB, true);
        intent.putExtra(Browser.EXTRA_APPLICATION_ID, mActivity.getPackageName());
        IntentHandler.addTrustedIntentExtras(intent);
        IntentHandler.setTabLaunchType(intent, TabLaunchType.FROM_EXTERNAL_APP);
        IntentUtils.safeStartActivity(mActivity, intent);
    }

    /**
     * Called when the {@code linkUrl} should be opened in Chrome in the background.
     * @param linkUrl The url to open.
     */
    public void onOpenInBackground(String linkUrl) {
        BrowserActionsService.sendIntent(
                BrowserActionsService.ACTION_TAB_CREATION_START, Tab.INVALID_TAB_ID);

        int tabId = openTabInBackground(linkUrl);
        if (tabId != Tab.INVALID_TAB_ID) {
            BrowserActionsService.sendIntent(
                    BrowserActionsService.ACTION_TAB_CREATION_UPDATE, Tab.INVALID_TAB_ID);
        }
        Toast.makeText(mActivity, R.string.browser_actions_open_in_background_toast_message,
                     Toast.LENGTH_SHORT)
                .show();
    }

    /**
     * Called when a custom item of Browser action menu is selected.
     * @param action The PendingIntent action to be launched.
     */
    public void onCustomItemSelected(PendingIntent action) {
        try {
            action.send();
        } catch (CanceledException e) {
            Log.e(TAG, "Browser Action in Chrome failed to send pending intent.");
        }
    }

    /**
     * Called when the page of the {@code linkUrl} should be downloaded.
     * @param linkUrl The url of the page to download.
     */
    public void startDownload(String linkUrl) {}

    /**
     * Called when the {@code linkUrl} should be shared.
     * @param shareDirectly Whether to share directly with the previous app shared with.
     * @param linkUrl The url to share.
     */
    public void share(Boolean shareDirectly, String linkUrl) {
        ShareParams params = new ShareParams.Builder(mActivity, linkUrl, linkUrl)
                                     .setShareDirectly(shareDirectly)
                                     .setSaveLastUsed(!shareDirectly)
                                     .setSourcePackageName(mSourcePackageName)
                                     .setIsExternalUrl(true)
                                     .build();
        ShareHelper.share(params);
    }
}
