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
import org.chromium.chrome.browser.signin.SigninAccessPoint;
import org.chromium.chrome.browser.signin.SigninAndSyncView;
import org.chromium.chrome.browser.signin.SigninManager;
import org.chromium.chrome.browser.signin.SigninManager.SignInStateObserver;
import org.chromium.chrome.browser.signin.SigninPromoController;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.sync.AndroidSyncSettings;
import org.chromium.components.sync.AndroidSyncSettings.AndroidSyncSettingsObserver;

/**
 * Class that manages all the logic and UI behind the signin promo header in the bookmark
 * content UI. The header is shown only on certain situations, (e.g., not signed in).
 */
class BookmarkPromoHeader implements AndroidSyncSettingsObserver,
        SignInStateObserver {
    /**
     * Interface to listen signin promo header visibility changes.
     */
    interface PromoHeaderShowingChangeListener {
        /**
         * Called when signin promo header visibility is changed.
         */
        void onPromoHeaderShowingChanged();
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
    private PromoHeaderShowingChangeListener mShowingChangeListener;

    /**
     * Initializes the class. Note that this will start listening to signin related events and
     * update itself if needed.
     */
    BookmarkPromoHeader(Context context, PromoHeaderShowingChangeListener showingChangeListener,
            SigninPromoController controller) {
        mContext = context;
        mShowingChangeListener = showingChangeListener;

        AndroidSyncSettings.registerObserver(mContext, this);

        mSignInManager = SigninManager.get(mContext);
        mSignInManager.addSignInStateObserver(this);

        updateShouldShow(false);
        if (shouldShow()) {
            if (SigninPromoController.arePromosEnabled()) {
                Account[] accounts = AccountManagerFacade.get().tryGetGoogleAccounts();
                String defaultAccountName = accounts.length == 0 ? null : accounts[0].name;
                controller.setAccountName(defaultAccountName);
                controller.recordSigninPromoImpression();
            } else {
                int promoShowCount = ContextUtils.getAppSharedPreferences().getInt(
                                             PREF_SIGNIN_PROMO_SHOW_COUNT, 0)
                        + 1;
                ContextUtils.getAppSharedPreferences()
                        .edit()
                        .putInt(PREF_SIGNIN_PROMO_SHOW_COUNT, promoShowCount)
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
        View view;

        if (SigninPromoController.arePromosEnabled()) {
            view = LayoutInflater.from(mContext).inflate(
                    R.layout.signin_promo_view_bookmarks, parent, false);
        } else {
            SigninAndSyncView.Listener listener = new SigninAndSyncView.Listener() {
                @Override
                public void onViewDismissed() {
                    setSigninPromoDeclined();
                }
            };

            view = SigninAndSyncView.create(parent, listener, SigninAccessPoint.BOOKMARK_MANAGER);
            // A MarginResizer is used to apply margins in regular and wide display modes. Remove
            // the view's lateral padding so that margins can be used instead.
            ApiCompatibilityUtils.setPaddingRelative(
                    view, 0, view.getPaddingTop(), 0, view.getPaddingBottom());
        }

        return new ViewHolder(view) {};
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
    void setSigninPromoDeclined() {
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

        if (SigninPromoController.arePromosEnabled()) {
            mShouldShow &=
                    SigninPromoController.shouldShowPromo(SigninAccessPoint.BOOKMARK_MANAGER);

        } else {
            mShouldShow &=
                    ContextUtils.getAppSharedPreferences().getInt(PREF_SIGNIN_PROMO_SHOW_COUNT, 0)
                    < MAX_SIGNIN_PROMO_SHOW_COUNT;
        }

        if (oldIsShowing != mShouldShow && notifyUI) {
            mShowingChangeListener.onPromoHeaderShowingChanged();
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

    /**
     * Forces the promo to show for testing purposes.
     */
    @VisibleForTesting
    public static void setShouldShowForTests() {
        sShouldShowForTests = true;
    }
}
