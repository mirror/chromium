// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.accounts.Account;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
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
    private static final String IMPRESSIONS = "impressions";
    private static final int MAX_IMPRESSIONS = 20;

    public static boolean shouldShowPromo() {
        int noOfImpressions = ContextUtils.getAppSharedPreferences().getInt(IMPRESSIONS, 0);
        return noOfImpressions < MAX_IMPRESSIONS;
    }

    public static void recordSigninPromoImpression() {
        SharedPreferences preferences = ContextUtils.getAppSharedPreferences();
        int noOfImpressions = preferences.getInt(IMPRESSIONS, 0);
        preferences.edit().putInt(IMPRESSIONS, noOfImpressions + 1).apply();
    }

    private final ProfileDataCache mProfileDataCache;
    private final @AccountSigninActivity.AccessPoint int mAccessPoint;

    /**
     * Receives notifications when user clicks close button in the promo.
     */
    public interface OnDismissListener { void onDismiss(); }

    public SigninPromoMediator(
            ProfileDataCache profileDataCache, @AccountSigninActivity.AccessPoint int accessPoint) {
        mProfileDataCache = profileDataCache;
        mAccessPoint = accessPoint;
    }

    /**
     * Configures the signin promo view.
     * @param view              The view in which the promo will be added.
     * @param onDismissListener Listener which handles the action of dismissing the promo. A null
     *                          onDismissListener marks that the promo is not dismissible and it
     *                          hides the close button (X)
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
        String signinText = context.getString(R.string.sign_in_button);
        Drawable chromeIcon = context.getResources().getDrawable(R.drawable.chrome_sync_logo);

        view.getSigninButton().setText(signinText);

        setImageSize(context, view, R.dimen.signin_promo_cold_state_image_size);
        view.getPromoImage().setImageDrawable(chromeIcon);

        view.getSigninButton().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint);
            }
        });

        view.getChooseButton().setVisibility(View.GONE);
    }

    private void setupHotState(final Context context, SigninPromoView view, String accountName) {
        mProfileDataCache.update(Collections.singletonList(accountName));

        String accountTitle = getAccountTitle(accountName);
        String signinButtonText =
                context.getString(R.string.signin_promo_continue_as, accountTitle);
        String chooseAccountButtonText =
                context.getString(R.string.signin_promo_choose_account, accountName);
        Drawable accountImage = mProfileDataCache.getImage(accountName);

        view.getSigninButton().setText(signinButtonText);
        view.getChooseButton().setText(chooseAccountButtonText);

        setImageSize(context, view, R.dimen.signin_promo_account_image_size);
        view.getPromoImage().setImageDrawable(accountImage);

        view.getSigninButton().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint);
            }
        });

        view.getChooseButton().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint);
            }
        });

        view.getChooseButton().setVisibility(View.VISIBLE);
    }

    private void setImageSize(Context context, SigninPromoView view, int size) {
        ViewGroup.LayoutParams layoutParams = view.getPromoImage().getLayoutParams();
        layoutParams.height = context.getResources().getDimensionPixelOffset(size);
        layoutParams.width = context.getResources().getDimensionPixelOffset(size);
    }

    private String getAccountName() {
        Account[] accounts = AccountManagerHelper.get().tryGetGoogleAccounts();
        return accounts.length == 0 ? null : accounts[0].name;
    }

    private String getAccountTitle(String accountName) {
        String title = AccountManagementFragment.getCachedUserName(accountName);

        if (title == null) {
            title = mProfileDataCache.getFullName(accountName);
        }

        return title;
    }
}
