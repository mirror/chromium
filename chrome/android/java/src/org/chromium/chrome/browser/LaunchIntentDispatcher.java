// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.Notification;
import android.app.SearchManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.provider.Browser;
import android.support.customtabs.CustomTabsIntent;
import android.text.TextUtils;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.metrics.CachedMetrics;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider;
import org.chromium.chrome.browser.customtabs.SeparateTaskCustomTabActivity;
import org.chromium.chrome.browser.dom_distiller.ReaderModeManager;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;
import org.chromium.chrome.browser.instantapps.InstantAppsHandler;
import org.chromium.chrome.browser.metrics.MediaNotificationUma;
import org.chromium.chrome.browser.multiwindow.MultiInstanceChromeTabbedActivity;
import org.chromium.chrome.browser.multiwindow.MultiWindowUtils;
import org.chromium.chrome.browser.notifications.NotificationPlatformBridge;
import org.chromium.chrome.browser.partnercustomizations.PartnerBrowserCustomizations;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.DocumentModeAssassin;
import org.chromium.chrome.browser.upgrade.UpgradeActivity;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.browser.util.UrlUtilities;
import org.chromium.chrome.browser.vr.VrMainActivity;
import org.chromium.chrome.browser.vr_shell.VrIntentUtils;
import org.chromium.chrome.browser.webapps.ActivityAssigner;
import org.chromium.chrome.browser.webapps.WebappLauncherActivity;
import org.chromium.ui.widget.Toast;

import java.lang.ref.WeakReference;
import java.net.URI;
import java.util.UUID;

/**
 * Dispatches incoming intents to the appropriate activity based on the current configuration and
 * Intent fired.
 */
public class LaunchIntentDispatcher implements IntentHandler.IntentHandlerDelegate {
    /**
     * Extra indicating launch mode used.
     */
    public static final String EXTRA_LAUNCH_MODE =
            "com.google.android.apps.chrome.EXTRA_LAUNCH_MODE";

    /**
     * Whether or not the toolbar should indicate that a tab was spawned by another Activity.
     */
    public static final String EXTRA_IS_ALLOWED_TO_RETURN_TO_PARENT =
            "org.chromium.chrome.browser.document.IS_ALLOWED_TO_RETURN_TO_PARENT";

    private static final String TAG = "ActivitiyDispatcher";

    /**
     * Timeout in ms for reading PartnerBrowserCustomizations provider. We do not trust third party
     * provider by default.
     */
    private static final int PARTNER_BROWSER_CUSTOMIZATIONS_TIMEOUT_MS = 10000;

    private static final CachedMetrics.SparseHistogramSample sIntentFlagsHistogram =
            new CachedMetrics.SparseHistogramSample("Launch.IntentFlags");

    private Activity mActivity;
    private Intent mIntent;

    private IntentHandler mIntentHandler;
    private boolean mIsInLegacyMultiInstanceMode;

    private boolean mIsCustomTabIntent;
    private boolean mIsHerbIntent;

    public LaunchIntentDispatcher(Activity activity, Intent intent) {
        mActivity = activity;
        mIntent = intent;
    }

    public enum Action {
        CONTINUE,
        FINISH,
        FINISH_REMOVE_TASK,
    }

    /**
     * Figure out how to route the Intent.  Because this is on the critical path to startup, please
     * avoid making the pathway any more complicated than it already is.  Make sure that anything
     * you add _absolutely has_ to be here.
     */
    public Action dispatch() {
        mIntent = IntentUtils.sanitizeIntent(mIntent);

        // Needs to be called as early as possible, to accurately capture the
        // time at which the intent was received.
        if (mIntent != null && IntentHandler.getTimestampFromIntent(mIntent) == -1) {
            IntentHandler.addTimestampToIntent(mIntent);
        }

        // Read partner browser customizations information asynchronously.
        // We want to initialize early because when there is no tabs to restore, we should possibly
        // show homepage, which might require reading PartnerBrowserCustomizations provider.
        PartnerBrowserCustomizations.initializeAsync(
                mActivity.getApplicationContext(), PARTNER_BROWSER_CUSTOMIZATIONS_TIMEOUT_MS);
        recordIntentMetrics();

        mIsInLegacyMultiInstanceMode = shouldRunInLegacyMultiInstanceMode();
        mIntentHandler = new IntentHandler(this, mActivity.getPackageName());
        mIsCustomTabIntent = isCustomTabIntent(mIntent);
        boolean isVrIntent = VrIntentUtils.isVrIntent(mIntent);
        // If the intent was created by Reader Mode, ignore herb and custom tab information.
        if (!mIsCustomTabIntent && !ReaderModeManager.isReaderModeCreatedIntent(mIntent)
                && !isVrIntent) {
            mIsHerbIntent = isHerbIntent();
            mIsCustomTabIntent = mIsHerbIntent;
        }

        int tabId = IntentUtils.safeGetIntExtra(
                mIntent, IntentHandler.TabOpenType.BRING_TAB_TO_FRONT.name(), Tab.INVALID_TAB_ID);
        boolean incognito =
                mIntent.getBooleanExtra(IntentHandler.EXTRA_OPEN_NEW_INCOGNITO_TAB, false);

        // Check if a web search Intent is being handled.
        String url = IntentHandler.getUrlFromIntent(mIntent);
        if (url == null && tabId == Tab.INVALID_TAB_ID && !incognito
                && mIntentHandler.handleWebSearchIntent(mIntent)) {
            return Action.FINISH;
        }

        // Check if a LIVE WebappActivity has to be brought back to the foreground.  We can't
        // check for a dead WebappActivity because we don't have that information without a global
        // TabManager.  If that ever lands, code to bring back any Tab could be consolidated
        // here instead of being spread between ChromeTabbedActivity and ChromeLauncherActivity.
        // https://crbug.com/443772, https://crbug.com/522918
        if (WebappLauncherActivity.bringWebappToFront(tabId)) {
            return Action.FINISH_REMOVE_TASK;
        }

        // The notification settings cog on the flipped side of Notifications and in the Android
        // Settings "App Notifications" view will open us with a specific category.
        if (mIntent.hasCategory(Notification.INTENT_CATEGORY_NOTIFICATION_PREFERENCES)) {
            NotificationPlatformBridge.launchNotificationPreferences(mActivity, mIntent);
            return Action.FINISH;
        }

        // Check if we should launch an Instant App to handle the intent.
        if (InstantAppsHandler.getInstance().handleIncomingIntent(
                    mActivity, mIntent, mIsCustomTabIntent && !mIsHerbIntent, false)) {
            return Action.FINISH;
        }

        // Check if we should push the user through First Run.
        if (FirstRunFlowSequencer.launch(mActivity, mIntent, false /* requiresBroadcast */,
                    false /* preferLightweightFre */)) {
            return Action.FINISH;
        }

        // Check if we should launch the ChromeTabbedActivity.
        if (!mIsCustomTabIntent && !FeatureUtilities.isDocumentMode(mActivity)) {
            return launchTabbedMode();
        }

        // Check if we should launch a Custom Tab.
        if (mIsCustomTabIntent) {
            launchCustomTabActivity();
            return Action.FINISH;
        }

        // Force a user to migrate to document mode, if necessary.
        if (DocumentModeAssassin.getInstance().isMigrationNecessary()) {
            Log.d(TAG, "Diverting to UpgradeActivity via ChromeLauncherActivity.");
            UpgradeActivity.launchInstance(mActivity, mIntent);
            return Action.FINISH_REMOVE_TASK;
        }

        return Action.CONTINUE;
    }

    @Override
    public void processWebSearchIntent(String query) {
        Intent searchIntent = new Intent(Intent.ACTION_WEB_SEARCH);
        searchIntent.putExtra(SearchManager.QUERY, query);
        mActivity.startActivity(searchIntent);
    }

    @Override
    public void processUrlViewIntent(String url, String referer, String headers,
            IntentHandler.TabOpenType tabOpenType, String externalAppId, int tabIdToBringToFront,
            boolean hasUserGesture, Intent intent) {
        assert false;
    }

    /** When started with an intent, maybe pre-resolve the domain. */
    private void maybePrefetchDnsInBackground() {
        if (mIntent != null && Intent.ACTION_VIEW.equals(mIntent.getAction())) {
            String maybeUrl = IntentHandler.getUrlFromIntent(mIntent);
            if (maybeUrl != null) {
                WarmupManager.getInstance().maybePrefetchDnsForUrlInBackground(mActivity, maybeUrl);
            }
        }
    }

    /**
     * @return Whether or not an Herb prototype may hijack an Intent.
     */
    private static boolean canBeHijackedByHerb(Intent intent) {
        String url = IntentHandler.getUrlFromIntent(intent);

        // Only VIEW Intents with URLs are rerouted to Custom Tabs.
        if (intent == null || !TextUtils.equals(Intent.ACTION_VIEW, intent.getAction())
                || TextUtils.isEmpty(url)) {
            return false;
        }

        // Don't open explicitly opted out intents in custom tabs.
        if (CustomTabsIntent.shouldAlwaysUseBrowserUI(intent)) {
            return false;
        }

        // Don't reroute Chrome Intents.
        Context context = ContextUtils.getApplicationContext();
        if (TextUtils.equals(context.getPackageName(),
                    IntentUtils.safeGetStringExtra(intent, Browser.EXTRA_APPLICATION_ID))
                || IntentHandler.wasIntentSenderChrome(intent)) {
            return false;
        }

        // Don't reroute internal chrome URLs.
        try {
            URI uri = URI.create(url);
            if (UrlUtilities.isInternalScheme(uri)) return false;
        } catch (IllegalArgumentException e) {
            return false;
        }

        // Don't reroute Home screen shortcuts.
        if (IntentUtils.safeHasExtra(intent, ShortcutHelper.EXTRA_SOURCE)) {
            return false;
        }

        // Don't reroute intents created by Reader Mode.
        if (ReaderModeManager.isReaderModeCreatedIntent(intent)) {
            return false;
        }

        return true;
    }

    /**
     * @return Whether or not a Custom Tab will be forcefully used for the incoming Intent.
     */
    private boolean isHerbIntent() {
        if (!canBeHijackedByHerb(mIntent)) return false;

        // Different Herb flavors handle incoming intents differently.
        String flavor = FeatureUtilities.getHerbFlavor();
        if (TextUtils.isEmpty(flavor)
                || TextUtils.equals(ChromeSwitches.HERB_FLAVOR_DISABLED, flavor)) {
            return false;
        } else if (TextUtils.equals(flavor, ChromeSwitches.HERB_FLAVOR_ELDERBERRY)) {
            return IntentUtils.safeGetBooleanExtra(
                    mIntent, EXTRA_IS_ALLOWED_TO_RETURN_TO_PARENT, true);
        } else {
            // Legacy Herb Flavors might hit this path before the caching logic corrects it, so
            // treat this as disabled.
            return false;
        }
    }

    /**
     * @return Whether the intent is for launching a Custom Tab.
     */
    public static boolean isCustomTabIntent(Intent intent) {
        if (intent == null) return false;
        if ((CustomTabsIntent.shouldAlwaysUseBrowserUI(intent)
                    || !intent.hasExtra(CustomTabsIntent.EXTRA_SESSION))
                && !VrIntentUtils.isCustomTabVrIntent(intent)) {
            return false;
        }
        return IntentHandler.getUrlFromIntent(intent) != null;
    }

    /**
     * Creates an Intent that can be used to launch a {@link CustomTabActivity}.
     */
    public static Intent createCustomTabActivityIntent(
            Context context, Intent intent, boolean addHerbExtras) {
        // Use the copy constructor to carry over the myriad of extras.
        Uri uri = Uri.parse(IntentHandler.getUrlFromIntent(intent));

        Intent newIntent = new Intent(intent);
        newIntent.setAction(Intent.ACTION_VIEW);
        newIntent.setClassName(context, CustomTabActivity.class.getName());
        newIntent.setData(uri);

        // If a CCT intent triggers First Run, then NEW_TASK will be automatically applied.  As
        // part of that, it will inherit the EXCLUDE_FROM_RECENTS bit from ChromeLauncherActivity,
        // so explicitly remove it to ensure the CCT does not get lost in recents.
        if ((newIntent.getFlags() & Intent.FLAG_ACTIVITY_NEW_TASK) != 0
                || (newIntent.getFlags() & Intent.FLAG_ACTIVITY_NEW_DOCUMENT) != 0) {
            newIntent.setFlags(newIntent.getFlags() & ~Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
            String uuid = UUID.randomUUID().toString();
            newIntent.addFlags(Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                // Force a new document L+ to ensure the proper task/stack creation.
                newIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
                if (VrIntentUtils.isVrIntent(intent)) {
                    newIntent.setClassName(context, VrMainActivity.class.getName());
                } else {
                    newIntent.setClassName(context, SeparateTaskCustomTabActivity.class.getName());
                }
            } else {
                int activityIndex =
                        ActivityAssigner.instance(ActivityAssigner.SEPARATE_TASK_CCT_NAMESPACE)
                                .assign(uuid);
                String className = SeparateTaskCustomTabActivity.class.getName() + activityIndex;
                newIntent.setClassName(context, className);
            }

            String url = IntentHandler.getUrlFromIntent(newIntent);
            assert url != null;
            newIntent.setData(new Uri.Builder()
                                      .scheme(UrlConstants.CUSTOM_TAB_SCHEME)
                                      .authority(uuid)
                                      .query(url)
                                      .build());
        }

        if (addHerbExtras) {
            // TODO(tedchoc|mariakhomenko): Specifically not marking the intent is from Chrome via
            //                              IntentHandler.addTrustedIntentExtras as it breaks the
            //                              redirect logic for triggering instant apps.  See if
            //                              this is better addressed in TabRedirectHandler long
            //                              term.
            newIntent.putExtra(CustomTabIntentDataProvider.EXTRA_IS_OPENED_BY_CHROME, true);
            newIntent.putExtra(CustomTabsIntent.EXTRA_DEFAULT_SHARE_MENU_ITEM, true);
        } else {
            IntentUtils.safeRemoveExtra(
                    intent, CustomTabIntentDataProvider.EXTRA_IS_OPENED_BY_CHROME);
        }

        return newIntent;
    }

    /**
     * Handles launching a {@link CustomTabActivity}, which will sit on top of a client's activity
     * in the same task.
     */
    private void launchCustomTabActivity() {
        boolean handled = CustomTabActivity.handleInActiveContentIfNeeded(mIntent);
        if (handled) return;

        maybePrefetchDnsInBackground();

        // Create and fire a launch intent.
        Bundle options = null;
        if (VrIntentUtils.isVrIntent(mIntent)) {
            // VR intents will open a VR-specific CCT {@link VrMainActivity} which
            // starts with a theme that disables the system preview window. As a side effect, you
            // see a flash of the previous app exiting before Chrome is started. These options
            // prevent that flash as it can look jarring while the user is in their headset.
            options = VrIntentUtils.getVrIntentOptions(mActivity);
        }
        mActivity.startActivity(createCustomTabActivityIntent(mActivity, mIntent,
                                        !isCustomTabIntent(mIntent) && mIsHerbIntent),
                options);
        if (mIsHerbIntent) {
            mActivity.overridePendingTransition(org.chromium.chrome.R.anim.activity_open_enter,
                    org.chromium.chrome.R.anim.no_anim);
        }
    }

    /**
     * Handles launching a {@link ChromeTabbedActivity}.
     */
    @SuppressLint("InlinedApi")
    private Action launchTabbedMode() {
        maybePrefetchDnsInBackground();

        Intent newIntent = new Intent(mIntent);
        Class<?> tabbedActivityClass =
                MultiWindowUtils.getInstance().getTabbedActivityForIntent(newIntent, mActivity);
        newIntent.setClassName(
                mActivity.getApplicationContext().getPackageName(), tabbedActivityClass.getName());
        newIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            newIntent.addFlags(Intent.FLAG_ACTIVITY_RETAIN_IN_RECENTS);
        }
        Uri uri = newIntent.getData();
        boolean isContentScheme = false;
        if (uri != null && UrlConstants.CONTENT_SCHEME.equals(uri.getScheme())) {
            isContentScheme = true;
            newIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        }
        if (mIsInLegacyMultiInstanceMode) {
            makeLegacyMultiInstanceIntent(newIntent);
        }

        android.util.Log.w("LAUNCHER-DBG",
                "launchTabbedMode() wants to start " + newIntent + ", activity already has "
                        + mIntent + ", and is " + mActivity.getClass().getName());

        if (newIntent.getComponent().getClassName().equals(mActivity.getClass().getName())) {
            // We're trying to start activity that is already running - just continue.
            return Action.CONTINUE;
        }

        // This system call is often modified by OEMs and not actionable. http://crbug.com/619646.
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();
        try {
            mActivity.startActivity(newIntent);
        } catch (SecurityException ex) {
            if (isContentScheme) {
                Toast.makeText(mActivity,
                             org.chromium.chrome.R.string.external_app_restricted_access_error,
                             Toast.LENGTH_LONG)
                        .show();
            } else {
                throw ex;
            }
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }

        return Action.FINISH;
    }

    /**
     * @return Whether there is already an browser instance of Chrome already running.
     */
    public boolean isChromeBrowserActivityRunning() {
        for (WeakReference<Activity> reference : ApplicationStatus.getRunningActivities()) {
            Activity activity = reference.get();
            if (activity == null) continue;

            String className = activity.getClass().getName();
            if (TextUtils.equals(className, ChromeTabbedActivity.class.getName())) {
                return true;
            }
        }
        return false;
    }

    /**
     * Records metrics gleaned from the Intent.
     */
    private void recordIntentMetrics() {
        IntentHandler.ExternalAppId source =
                IntentHandler.determineExternalIntentSource(mActivity.getPackageName(), mIntent);
        if (mIntent.getPackage() == null && source != IntentHandler.ExternalAppId.CHROME) {
            int flagsOfInterest = Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_NEW_DOCUMENT;
            int maskedFlags = mIntent.getFlags() & flagsOfInterest;
            sIntentFlagsHistogram.record(maskedFlags);
        }
        MediaNotificationUma.recordClickSource(mIntent);
    }

    /**
     * @return Whether or not activity should run in pre-N Samsung multi-instance mode.
     */
    private boolean shouldRunInLegacyMultiInstanceMode() {
        return Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP
                && TextUtils.equals(mIntent.getAction(), Intent.ACTION_MAIN)
                && MultiWindowUtils.getInstance().isLegacyMultiWindow(mActivity)
                && isChromeBrowserActivityRunning();
    }

    /**
     * Makes |intent| able to support multi-instance in pre-N Samsung multi-window mode.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private void makeLegacyMultiInstanceIntent(Intent intent) {
        if (MultiWindowUtils.getInstance().isLegacyMultiWindow(mActivity)) {
            if (TextUtils.equals(ChromeTabbedActivity.class.getName(),
                        intent.getComponent().getClassName())) {
                intent.setClassName(mActivity, MultiInstanceChromeTabbedActivity.class.getName());
            }
            intent.setFlags(intent.getFlags()
                    & ~(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_NEW_DOCUMENT));
        }
    }
}
