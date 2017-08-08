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

    private static final String PREF_SIGNIN_PROMO_DECLINED =
            "enhanced_bookmark_signin_promo_declined";
    private static final String PREF_SIGNIN_PROMO_SHOW_COUNT =
            "enhanced_bookmark_signin_promo_show_count";
    // TODO(kkimlabs): Figure out the optimal number based on UMA data.
    private static final int MAX_SIGNIN_PROMO_SHOW_COUNT = 10;

    private static boolean sShouldShowForTests;

    private Context mContext;
    private SigninManager mSignInManager;
    private boolean mShouldShow;
    private PromoHeaderChangeListener mShowingChangeListener;
    private BookmarkDelegate mBookmarkDelegate;
    private SigninPromoController mSigninPromoController;
    private ProfileDataCache mProfileDataCache;

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
        if (shouldShow()) {
            if (SigninPromoController.shouldShowPromo(SigninAccessPoint.BOOKMARK_MANAGER)) {
                Account[] accounts = AccountManagerFacade.get().tryGetGoogleAccounts();
                String defaultAccountName = accounts.length == 0 ? null : accounts[0].name;
                mSigninPromoController.setAccountName(defaultAccountName);
                mSigninPromoController.recordSigninPromoImpression();
            } else {
                int promoShowCount = ContextUtils.getAppSharedPreferences().getInt(
                        PREF_SIGNIN_PROMO_SHOW_COUNT, 0);
                ContextUtils.getAppSharedPreferences()
                        .edit()
                        .putInt(PREF_SIGNIN_PROMO_SHOW_COUNT, promoShowCount + 1)
                        .apply();
                RecordUserAction.record("Signin_Impression_FromBookmarkManager");
            }
        }
    }

    /**
     * Clean ups the class.  Must be called once done using this class.
     */
    void destroy() {
        AndroidSyncSettings.unregisterObserver(mContext, this);

        if (SigninPromoController.shouldShowPromo(SigninAccessPoint.BOOKMARK_MANAGER)) {
            AccountManagerFacade.get().removeObserver(this);
            mProfileDataCache.removeObserver(this);
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
     * @return Signin promo header {@link ViewHolder} instance that can be used with
     *         {@link RecyclerView}.
     */
    ViewHolder createHolder(ViewGroup parent) {
        if (SigninPromoController.shouldShowPromo(SigninAccessPoint.BOOKMARK_MANAGER)) {
            View view = LayoutInflater.from(mContext).inflate(
                    R.layout.signin_promo_view_bookmarks, parent, false);
            return new ViewHolder(view) {};
        } else {
            SigninAndSyncView.Listener listener = new SigninAndSyncView.Listener() {
                @Override
                public void onViewDismissed() {
                    setSigninPromoDeclined();
                }
            };

            View view =
                    SigninAndSyncView.create(parent, listener, SigninAccessPoint.BOOKMARK_MANAGER);
            // A MarginResizer is used to apply margins in regular and wide display modes. Remove
            // the view's lateral padding so that margins can be used instead.
            ApiCompatibilityUtils.setPaddingRelative(
                    view, 0, view.getPaddingTop(), 0, view.getPaddingBottom());
            MarginResizer.createAndAttach(view,
                    mBookmarkDelegate.getSelectableListLayout().getUiConfig(),
                    parent.getResources().getDimensionPixelSize(
                            R.dimen.signin_and_sync_view_padding),
                    SelectableListLayout.getDefaultListItemLateralShadowSizePx(
                            parent.getResources()));
            return new ViewHolder(view) {};
        }
    }

    /**
     * @return Whether user tapped "No" button on the signin promo header.
     */
    private boolean wasSigninPromoDeclined() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_SIGNIN_PROMO_DECLINED, false);
    }

    /**
     * Saves that the promo was dismissed and updates the UI.
     */
    private void setSigninPromoDeclined() {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_SIGNIN_PROMO_DECLINED, true);
        sharedPreferencesEditor.apply();
        updateShouldShow(true);
    }

    private void updateShouldShow(boolean notifyUI) {
        boolean oldIsShowing = mShouldShow;

        mShouldShow = mSignInManager.isSignInAllowed() && !wasSigninPromoDeclined()
                && AndroidSyncSettings.isMasterSyncEnabled(mContext);

        if (ChromeFeatureList.isEnabled(ChromeFeatureList.ANDROID_SIGNIN_PROMOS)) {
            mShouldShow &=
                    SigninPromoController.shouldShowPromo(SigninAccessPoint.BOOKMARK_MANAGER);

        } else {
            mShouldShow &=
                    ContextUtils.getAppSharedPreferences().getInt(PREF_SIGNIN_PROMO_SHOW_COUNT, 0)
                    < MAX_SIGNIN_PROMO_SHOW_COUNT;
        }

        if (oldIsShowing != mShouldShow && notifyUI) {
            mShowingChangeListener.onPromoHeaderChanged();
        }
    }

    void setupSigninPromo(SigninPromoView view) {
        Account[] accounts = AccountManagerFacade.get().tryGetGoogleAccounts();
        String defaultAccountName = accounts.length == 0 ? null : accounts[0].name;

        if (defaultAccountName != null) {
            mProfileDataCache.update(Collections.singletonList(defaultAccountName));
        }

        mSigninPromoController.setAccountName(defaultAccountName);

        SigninPromoController.OnDismissListener listener =
                new SigninPromoController.OnDismissListener() {
                    @Override
                    public void onDismiss() {
                        setSigninPromoDeclined();
                    }
                };
        mSigninPromoController.setupSigninPromoView(mContext, view, listener);
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
