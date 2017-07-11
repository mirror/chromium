// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.accounts.Account;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.firstrun.ProfileDataCache;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileDownloader;
import org.chromium.components.signin.AccountManagerHelper;
import org.chromium.ui.widget.ButtonCompat;

import java.util.Collections;

/**
 * A mediator for configuring the sign in promo.
 */
public class SigninPromoMediator {
    private static final String PREFERENCES_NAME = "signin_promo_impressions";
    private static final String IMPRESSIONS = "impressions";
    private static final int MAX_IMPRESSIONS = 20;

    public static boolean shouldShowPromo(Context context) {
        int noOfImpressions = context.getSharedPreferences(PREFERENCES_NAME, Context.MODE_PRIVATE)
                                      .getInt(IMPRESSIONS, 0);
        return noOfImpressions < MAX_IMPRESSIONS;
    }

    public static void recordSigninPromoImpression(Context context) {
        SharedPreferences preferences =
                context.getSharedPreferences(PREFERENCES_NAME, Context.MODE_PRIVATE);
        int noOfImpressions = preferences.getInt(IMPRESSIONS, 0);
        preferences.edit().putInt(IMPRESSIONS, noOfImpressions + 1).apply();
    }

    // change this flag to true to test the cold state UI
    private boolean mTestColdState = false;

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
     * Sets up the sign in promo depending on the context: whether there are cached accounts which
     * have been previously signed in or not or whether the promo is dismissible or not.
     * @param view              The view in which the promo will be added.
     * @param onDismissListener Listener which handles the action of dismissing the promo. A null
     *                          onDismissListener marks that the promo is not dismissible.
     */
    public void setupSigninPromoView(
            View view, Context context, final OnDismissListener onDismissListener) {
        LinearLayout container = view.findViewById(R.id.signin_promo_layout_container);
        ButtonCompat continueButton = container.findViewById(R.id.signin_promo_button_continue);
        Button chooseAccountButton =
                container.findViewById(R.id.signin_promo_button_choose_account);
        ImageView promoImage = container.findViewById(R.id.signin_promo_image);

        String accountName = getAccountName();
        if (accountName == null || mTestColdState) {
            setUpColdState(context, continueButton, chooseAccountButton, promoImage);
        } else {
            setUpHotState(context, accountName, continueButton, chooseAccountButton, promoImage);
        }

        ImageButton dismissButton = container.findViewById(R.id.signin_promo_close_button);
        if (onDismissListener != null) {
            dismissButton.setVisibility(View.VISIBLE);
            dismissButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    onDismissListener.onDismiss();
                }
            });
        } else {
            dismissButton.setVisibility(View.GONE);
        }
    }

    private void setUpColdState(final Context context, ButtonCompat continueButton,
            Button chooseAccountButton, ImageView promoImage) {
        continueButton.setText(R.string.sign_in_button);

        setImageSize(context, promoImage, R.dimen.signin_promo_cold_state_image_size);
        promoImage.setImageResource(R.drawable.chrome_sync_logo);

        continueButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint);
            }
        });

        chooseAccountButton.setVisibility(View.GONE);
    }

    private void setUpHotState(final Context context, String accountName,
            ButtonCompat continueButton, Button chooseAccountButton, ImageView promoImage) {
        String accountTitle = getAccountTitle(context, accountName);

        String continueButtonText =
                context.getResources().getString(R.string.signin_promo_continue_as, accountTitle);
        continueButton.setText(continueButtonText);

        String chooseAccountButtonText =
                context.getResources().getString(R.string.signin_promo_choose_account, accountName);
        chooseAccountButton.setText(chooseAccountButtonText);

        setImageSize(context, promoImage, R.dimen.signin_promo_account_image_size);
        mProfileDataCache.update(Collections.singletonList(accountName));
        Drawable accountImage = mProfileDataCache.getImage(accountName);
        promoImage.setImageDrawable(accountImage);

        continueButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint);
            }
        });

        chooseAccountButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint);
            }
        });

        chooseAccountButton.setVisibility(View.VISIBLE);
    }

    private void setImageSize(Context context, ImageView promoImage, int size) {
        ViewGroup.LayoutParams layoutParams = promoImage.getLayoutParams();
        layoutParams.height = context.getResources().getDimensionPixelOffset(size);
        layoutParams.width = context.getResources().getDimensionPixelOffset(size);
    }

    private String getAccountName() {
        Account[] accounts = AccountManagerHelper.get().tryGetGoogleAccounts();
        return accounts.length == 0 ? null : accounts[0].name;
    }

    private String getAccountTitle(Context context, String accountName) {
        String title = AccountManagementFragment.getCachedUserName(accountName);

        if (title == null) {
            Profile profile = Profile.getLastUsedProfile();
            String cachedName = ProfileDownloader.getCachedFullName(profile);
            Bitmap cachedBitmap = ProfileDownloader.getCachedAvatar(profile);
            final int imageSidePixels = context.getResources().getDimensionPixelOffset(
                    R.dimen.signin_promo_account_image_size);

            if (TextUtils.isEmpty(cachedName) || cachedBitmap == null) {
                ProfileDownloader.startFetchingAccountInfoFor(
                        context, profile, accountName, imageSidePixels, true);
            }

            title = TextUtils.isEmpty(cachedName) ? accountName : cachedName;
        }

        return title;
    }
}
