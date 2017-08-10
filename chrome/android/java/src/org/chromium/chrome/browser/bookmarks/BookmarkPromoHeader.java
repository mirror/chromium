// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import android.accounts.Account;
import android.content.Context;
import android.content.SharedPreferences;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ViewSwitcher;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.signin.ProfileDataCache;
import org.chromium.chrome.browser.signin.SigninAccessPoint;
import org.chromium.chrome.browser.signin.SigninAndSyncView;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.browser.signin.SigninManager.SignInStateObserver;
import org.chromium.chrome.browser.signin.SigninPromoController;
import org.chromium.chrome.browser.signin.SigninPromoView;
import org.chromium.chrome.browser.widget.displaystyle.MarginResizer;
import org.chromium.chrome.browser.widget.selection.SelectableListLayout;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.AccountsChangeObserver;
import org.chromium.components.signin.ChromeSigninController;
import org.chromium.components.sync.AndroidSyncSettings;
import org.chromium.components.sync.AndroidSyncSettings.AndroidSyncSettingsObserver;

import java.util.Collections;

/**
 * Class that manages all the logic and UI behind the signin promo header in the bookmark
 * content UI. The header is shown only on certain situations, (e.g., not signed in).
 */
class BookmarkPromoHeader implements AndroidSyncSettingsObserver, SignInStateObserver,
                                     ProfileDataCache.Observer, AccountsChangeObserver {
    /**
     * Interface to listen signin promo header changes.
     */
    interface PromoHeaderChangeListener {
        /**
         * Called when signin promo header is changed.
         */
        void onPromoHeaderChanged();
    }

    // New signin promo preferences.
    private static final String PREF_NEW_SIGNIN_PROMO_DECLINED = "signin_promo_bookmarks_declined";
    // Old signin promo preferences.
    private static final String PREF_OLD_SIGNIN_PROMO_DECLINED =
            "enhanced_bookmark_signin_promo_declined";
    private static final String PREF_SIGNIN_PROMO_SHOW_COUNT =
            "enhanced_bookmark_signin_promo_show_count";
    // TODO(kkimlabs): Figure out the optimal number based on UMA data.
    private static final int MAX_SIGNIN_PROMO_SHOW_COUNT = 5;

    private static boolean sShouldShowForTests;

    private Context mContext;
    private SigninManager mSignInManager;
    private boolean mShouldShow;
    private PromoHeaderChangeListener mShowingChangeListener;
    private BookmarkDelegate mBookmarkDelegate;
    private SigninPromoController mSigninPromoController;
    private ProfileDataCache mProfileDataCache;
    private boolean mIsNewPromoShowing;
    private boolean mShouldShowOldPromo;
    private ViewGroup.LayoutParams mOldPromoLayoutParams;
    private ViewGroup.LayoutParams mNewPromoLayoutParams;

    /**
     * Initializes the class. Note that this will start listening to signin related events and
     * update itself if needed.
     */
    BookmarkPromoHeader(Context context, PromoHeaderChangeListener showingChangeListener,
            BookmarkDelegate bookmarkDelegate) {
        mContext = context;
        mShowingChangeListener = showingChangeListener;
        mBookmarkDelegate = bookmarkDelegate;

        AndroidSyncSettings.registerObserver(mContext, this);

        if (SigninPromoController.shouldShowPromo(SigninAccessPoint.BOOKMARK_MANAGER)) {
            int imageSize =
                    mContext.getResources().getDimensionPixelSize(R.dimen.user_picture_size);
            mProfileDataCache =
                    new ProfileDataCache(mContext, Profile.getLastUsedProfile(), imageSize);
            mProfileDataCache.addObserver(this);
            mSigninPromoController = new SigninPromoController(
                    mProfileDataCache, SigninAccessPoint.BOOKMARK_MANAGER);
            AccountManagerFacade.get().addObserver(this);
        }

        mSignInManager = SigninManager.get(mContext);
        mSignInManager.addSignInStateObserver(this);

        updateShouldShow(false);
        if (shouldShow() && mShouldShowOldPromo) {
            int promoShowCount =
                    ContextUtils.getAppSharedPreferences().getInt(PREF_SIGNIN_PROMO_SHOW_COUNT, 0);
            ContextUtils.getAppSharedPreferences()
                    .edit()
                    .putInt(PREF_SIGNIN_PROMO_SHOW_COUNT, promoShowCount + 1)
                    .apply();
            if (!ChromeSigninController.get().isSignedIn()) {
                RecordUserAction.record("Signin_Impression_FromBookmarkManager");
            }
        }
    }

    /**
     * Clean ups the class.  Must be called once done using this class.
     */
    void destroy() {
        AndroidSyncSettings.unregisterObserver(mContext, this);

        if (mSigninPromoController != null) {
            AccountManagerFacade.get().removeObserver(this);
            mProfileDataCache.removeObserver(this);
            mProfileDataCache = null;
            mSigninPromoController = null;
        }

        mSignInManager.removeSignInStateObserver(this);
        mSignInManager = null;
    }

    /**
     * @return Whether it should be showing.
     */
    boolean shouldShow() {
        return mShouldShow || sShouldShowForTests;
    }

    /**
     * Updates the information of the promo header and switches between the promos if needed.
     * @param holder The holder containing the view to be updated.
     */
    void bindHolder(ViewHolder holder) {
        ViewSwitcher viewSwitcher = (ViewSwitcher) holder.itemView;
        View currentView = viewSwitcher.getCurrentView();

        boolean shouldSwitchViews = (mShouldShowOldPromo && currentView instanceof SigninPromoView)
                || (!mShouldShowOldPromo && currentView instanceof SigninAndSyncView);
        if (shouldSwitchViews) {
            viewSwitcher.showNext();
        }

        if (mShouldShowOldPromo) {
            viewSwitcher.setLayoutParams(mOldPromoLayoutParams);
        } else {
            viewSwitcher.setLayoutParams(mNewPromoLayoutParams);
            setupSigninPromo((SigninPromoView) viewSwitcher.getCurrentView());
        }
    }

    /**
     * @return Signin promo header {@link ViewHolder} instance that can be used with
     *         {@link RecyclerView}.
     */
    ViewHolder createHolder(ViewGroup parent) {
        ViewSwitcher viewSwitcher = new ViewSwitcher(mContext);
        viewSwitcher.setMeasureAllChildren(false);

        SigninPromoView newPromo = createNewPromoView(parent);
        SigninAndSyncView oldPromo = createOldPromoView(parent);
        mNewPromoLayoutParams = newPromo.getLayoutParams();
        mOldPromoLayoutParams = oldPromo.getLayoutParams();
        viewSwitcher.addView(newPromo);
        viewSwitcher.addView(oldPromo);

        return new ViewHolder(viewSwitcher) {};
    }

    private SigninPromoView createNewPromoView(ViewGroup parent) {
        return (SigninPromoView) LayoutInflater.from(mContext).inflate(
                R.layout.signin_promo_view_bookmarks, parent, false);
    }

    private SigninAndSyncView createOldPromoView(ViewGroup parent) {
        SigninAndSyncView.Listener listener = new SigninAndSyncView.Listener() {
            @Override
            public void onViewDismissed() {
                setOldSigninPromoDeclined();
            }
        };

        SigninAndSyncView view =
                SigninAndSyncView.create(parent, listener, SigninAccessPoint.BOOKMARK_MANAGER);

        // A MarginResizer is used to apply margins in regular and wide display modes. Remove
        // the view's lateral padding so that margins can be used instead.
        ApiCompatibilityUtils.setPaddingRelative(
                view, 0, view.getPaddingTop(), 0, view.getPaddingBottom());
        MarginResizer.createAndAttach(view,
                mBookmarkDelegate.getSelectableListLayout().getUiConfig(),
                parent.getResources().getDimensionPixelSize(R.dimen.signin_and_sync_view_padding),
                SelectableListLayout.getDefaultListItemLateralShadowSizePx(parent.getResources()));
        return view;
    }

    /**
     * Saves that the new promo was declined and updates the UI.
     */
    private void setNewSigninPromoDeclined() {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_NEW_SIGNIN_PROMO_DECLINED, true);
        sharedPreferencesEditor.apply();
        updateShouldShow(true);
    }

    /**
     * Saves that the old promo was dismissed and updates the UI.
     */
    private void setOldSigninPromoDeclined() {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_OLD_SIGNIN_PROMO_DECLINED, true);
        sharedPreferencesEditor.apply();
        updateShouldShow(true);
    }

    /**
     * @return Whether the user declined the new signin promo.
     */
    private boolean wasNewSigninPromoDeclined() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_NEW_SIGNIN_PROMO_DECLINED, false);
    }

    /**
     * @return Whether user tapped "No" button on the old signin promo.
     */
    private boolean wasOldSigninPromoDeclined() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_OLD_SIGNIN_PROMO_DECLINED, false);
    }

    private void setupSigninPromo(SigninPromoView view) {
        Account[] accounts = AccountManagerFacade.get().tryGetGoogleAccounts();
        String defaultAccountName = accounts.length == 0 ? null : accounts[0].name;

        if (defaultAccountName != null) {
            mProfileDataCache.update(Collections.singletonList(defaultAccountName));
        }

        mSigninPromoController.setAccountName(defaultAccountName);
        if (!mIsNewPromoShowing) {
            mIsNewPromoShowing = true;
            mSigninPromoController.recordSigninPromoImpression();
        }

        SigninPromoController.OnDismissListener listener =
                new SigninPromoController.OnDismissListener() {
                    @Override
                    public void onDismiss() {
                        setNewSigninPromoDeclined();
                    }
                };
        mSigninPromoController.setupSigninPromoView(mContext, view, listener);
    }

    private void updateShouldShow(boolean notifyUI) {
        mShouldShow = calculateShouldShow();
        if (notifyUI) {
            mShowingChangeListener.onPromoHeaderChanged();
        }
    }

    private boolean calculateShouldShow() {
        if (!AndroidSyncSettings.isMasterSyncEnabled(mContext)) {
            mShouldShowOldPromo = false;
            return false;
        }

        // If the user is signed in, then we should show the sync promo if Chrome sync is disabled
        // and the impression limit has not been reached yet.
        if (ChromeSigninController.get().isSignedIn()) {
            mShouldShowOldPromo = true;
            boolean impressionLimitNotReached =
                    ContextUtils.getAppSharedPreferences().getInt(PREF_SIGNIN_PROMO_SHOW_COUNT, 0)
                    < MAX_SIGNIN_PROMO_SHOW_COUNT;
            return !AndroidSyncSettings.isChromeSyncEnabled(mContext) && impressionLimitNotReached;
        }

        // The signin promo should be shown if signin is allowed, it wasn't declined by the user
        // and the impression limit wasn't reached.
        if (ChromeFeatureList.isEnabled(ChromeFeatureList.ANDROID_SIGNIN_PROMOS)) {
            mShouldShowOldPromo = false;
            return mSignInManager.isSignInAllowed()
                    && SigninPromoController.shouldShowPromo(SigninAccessPoint.BOOKMARK_MANAGER)
                    && !wasNewSigninPromoDeclined();

        } else {
            mShouldShowOldPromo = true;
            boolean impressionLimitNotReached =
                    ContextUtils.getAppSharedPreferences().getInt(PREF_SIGNIN_PROMO_SHOW_COUNT, 0)
                    < MAX_SIGNIN_PROMO_SHOW_COUNT;
            return mSignInManager.isSignInAllowed() && impressionLimitNotReached
                    && !wasOldSigninPromoDeclined();
        }
    }

    // AndroidSyncSettingsObserver implementation

    @Override
    public void androidSyncSettingsChanged() {
        updateShouldShow(true);
    }

    // SignInStateObserver implementations

    @Override
    public void onSignedIn() {
        updateShouldShow(true);
    }

    @Override
    public void onSignedOut() {
        updateShouldShow(true);
    }

    // ProfileDataCacheObserver implementation.

    @Override
    public void onProfileDataUpdated(String accountId) {
        mShowingChangeListener.onPromoHeaderChanged();
    }

    // AccountsChangeObserver implementation.

    @Override
    public void onAccountsChanged() {
        mShowingChangeListener.onPromoHeaderChanged();
    }

    /**
     * Forces the promo to show for testing purposes.
     */
    @VisibleForTesting
    public static void setShouldShowForTests() {
        sShouldShowForTests = true;
    }
}
