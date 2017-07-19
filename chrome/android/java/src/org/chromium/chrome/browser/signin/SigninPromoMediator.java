// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.accounts.Account;
import android.annotation.DimenRes;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.firstrun.ProfileDataCache;
import org.chromium.components.signin.AccountManagerHelper;

import java.util.Collections;

import javax.annotation.Nullable;

/**
 * A mediator for configuring the sign in promo. It sets up the sign in promo depending on the
 * context: whether there are any Google accounts on the device which have been previously signed in
 * or not.
 */
public class SigninPromoMediator {
    /**
     * Receives notifications when user clicks close button in the promo.
     */
    public interface OnDismissListener { void onDismiss(); }

    private static final String PROMOS_SHOWN = "promos_shown";
    private static final int MAX_IMPRESSIONS = 20;

    /**
     * Determines whether the promo should be shown to the user or not.
     * @return true if the promo is to be shown and false otherwise.
     */
    public static boolean shouldShowPromo() {
        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.ANDROID_SIGNIN_PROMOS)) {
            return false;
        }

        int noOfImpressions = ContextUtils.getAppSharedPreferences().getInt(PROMOS_SHOWN, 0);
        return noOfImpressions < MAX_IMPRESSIONS;
    }

    /**
     * Records an impression for the promo.
     */
    public static void recordSigninPromoImpression() {
        SharedPreferences preferences = ContextUtils.getAppSharedPreferences();
        int noOfImpressions = preferences.getInt(PROMOS_SHOWN, 0);
        preferences.edit().putInt(PROMOS_SHOWN, noOfImpressions + 1).apply();
    }

    private final ProfileDataCache mProfileDataCache;
    private final @AccountSigninActivity.AccessPoint int mAccessPoint;

    /**
     * Creates a new SigninPromoMediator.
     * @param profileDataCache  The profile data cache for the latest used account on the device.
     * @param accessPoint       Specifies the AccessPoint from which the promo is to be shown.
     */
    public SigninPromoMediator(
            ProfileDataCache profileDataCache, @AccountSigninActivity.AccessPoint int accessPoint) {
        mProfileDataCache = profileDataCache;
        mAccessPoint = accessPoint;
    }

    /**
     * Configures the signin promo view.
     * @param view              The view in which the promo will be added.
     * @param onDismissListener Listener which handles the action of dismissing the promo. A null
     *         onDismissListener marks that the promo is not dismissible and as a result the close
     *         button is hidden.
     */
    public void setupSigninPromoView(Context context, SigninPromoView view,
            @Nullable final OnDismissListener onDismissListener) {
        String accountName = getAccountName();

        if (accountName == null) {
            setupColdState(context, view);
        } else {
            setupHotState(context, view, accountName);
        }

        if (onDismissListener != null) {
            view.getDismissButton().setVisibility(View.VISIBLE);
            view.getDismissButton().setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    onDismissListener.onDismiss();
                }
            });
        } else {
            view.getDismissButton().setVisibility(View.GONE);
        }
    }

    private void setupColdState(final Context context, SigninPromoView view) {
        String signinText = context.getString(R.string.sign_in_to_chrome);
        Drawable chromeIcon = context.getResources().getDrawable(R.drawable.chrome_sync_logo);

        setImageSize(context, view, R.dimen.signin_promo_cold_state_image_size);
        view.getImage().setImageDrawable(chromeIcon);

        view.getSigninButton().setText(signinText);
        view.getSigninButton().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint);
            }
        });

        view.getChooseAccountButton().setVisibility(View.GONE);
    }

    private void setupHotState(final Context context, SigninPromoView view, String accountName) {
        mProfileDataCache.update(Collections.singletonList(accountName));

        String accountTitle = getAccountTitle(accountName);
        String signinButtonText =
                context.getString(R.string.signin_promo_continue_as, accountTitle);
        String chooseAccountButtonText =
                context.getString(R.string.signin_promo_choose_account, accountName);
        Drawable accountImage = mProfileDataCache.getImage(accountName);

        setImageSize(context, view, R.dimen.signin_promo_account_image_size);
        view.getImage().setImageDrawable(accountImage);

        view.getSigninButton().setText(signinButtonText);
        view.getSigninButton().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint);
            }
        });

        view.getChooseAccountButton().setText(chooseAccountButtonText);
        view.getChooseAccountButton().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint);
            }
        });
        view.getChooseAccountButton().setVisibility(View.VISIBLE);
    }

    @Nullable
    private String getAccountName() {
        Account[] accounts = AccountManagerHelper.get().tryGetGoogleAccounts();
        return accounts.length == 0 ? null : accounts[0].name;
    }

    private String getAccountTitle(String accountName) {
        String title = mProfileDataCache.getFullName(accountName);
        return title == null ? accountName : title;
    }

    private void setImageSize(Context context, SigninPromoView view, @DimenRes int dimenResId) {
        ViewGroup.LayoutParams layoutParams = view.getImage().getLayoutParams();
        layoutParams.height = context.getResources().getDimensionPixelOffset(dimenResId);
        layoutParams.width = context.getResources().getDimensionPixelOffset(dimenResId);
    }
}
