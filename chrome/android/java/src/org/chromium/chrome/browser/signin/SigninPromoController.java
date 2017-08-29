// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.support.annotation.DimenRes;
import android.support.annotation.StringRes;
import android.view.View;
import android.view.ViewGroup;

import org.chromium.base.ContextUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.MainPreferences;
import org.chromium.chrome.browser.preferences.SignInPreference;

import javax.annotation.Nullable;

/**
 * A controller for configuring the sign in promo. It sets up the sign in promo depending on the
 * context: whether there are any Google accounts on the device which have been previously signed in
 * or not.
 */
public class SigninPromoController {
    /**
     * Receives notifications when user clicks close button in the promo.
     */
    public interface OnDismissListener {
        /**
         * Action to be performed when the promo is being dismissed.
         */
        void onDismiss();
    }

    private static final String SIGNIN_PROMO_IMPRESSIONS_COUNT_BOOKMARKS =
            "signin_promo_impressions_count_bookmarks";
    private static final String SIGNIN_PROMO_IMPRESSIONS_COUNT_SETTINGS =
            "signin_promo_impressions_count_settings";

    /**
     * The need of storing "wasUsed" comes from having to record
     * "MobileSignInPromo.[AccessPoint].ImpressionsTilDismiss". This histogram keeps track of the
     * number of users which have completely ignored the promo on the Xth impression (a.k.a no
     * buttons have been used and the promo wasn't actively dismissed).
     *
     * When wiring the personalized signin promo in Settings, an instance of the controller is
     * used in {@link SignInPreference}. Whenever the app is minimized or whenever the user presses
     * the signin buttons, the {@link MainPreferences} activity is put
     * {@link MainPreferences#onStop()} and when resumed, all preferences are destroyed and
     * then recreated. Therefore, the instance of the {@link SigninPromoController} will not last
     * for the whole duration of the Xth impression in some cases (e.g. the user presses the
     * "Continue as ..." button and then decides to "Undo" this operation or the app is temporarily
     * minimized). In order to not lose the information that one of the buttons has been pressed on
     * the Xth impression, we need to persist it.
     */
    private static final String SIGNIN_PROMO_WAS_USED = "signin_promo_was_used";
    private static final int MAX_IMPRESSIONS = 20;

    private String mAccountName;
    private final ProfileDataCache mProfileDataCache;
    private final @AccountSigninActivity.AccessPoint int mAccessPoint;
    private final @Nullable String mImpressionCountName;
    private final String mImpressionUserActionName;
    private final @Nullable String mImpressionWithAccountUserActionName;
    private final @Nullable String mImpressionWithNoAccountUserActionName;
    private final @StringRes int mDescriptionStringId;

    /**
     * Determines whether the promo should be shown to the user or not.
     * @param accessPoint The access point where the promo will be shown.
     * @return true if the promo is to be shown and false otherwise.
     */
    public static boolean shouldShowPromo(@AccountSigninActivity.AccessPoint int accessPoint) {
        if (!ChromeFeatureList.isEnabled(ChromeFeatureList.ANDROID_SIGNIN_PROMOS)) {
            return false;
        }

        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        switch (accessPoint) {
            case SigninAccessPoint.BOOKMARK_MANAGER:
                return sharedPreferences.getInt(SIGNIN_PROMO_IMPRESSIONS_COUNT_BOOKMARKS, 0)
                        < MAX_IMPRESSIONS;
            case SigninAccessPoint.NTP_CONTENT_SUGGESTIONS:
                // There is no impression limit for NTP content suggestions.
                return true;
            case SigninAccessPoint.RECENT_TABS:
                // There is no impression limit for Recent Tabs.
                return true;
            case SigninAccessPoint.SETTINGS:
                return sharedPreferences.getInt(SIGNIN_PROMO_IMPRESSIONS_COUNT_SETTINGS, 0)
                        < MAX_IMPRESSIONS;
            default:
                assert false : "Unexpected value for access point: " + accessPoint;
                return false;
        }
    }

    /**
     * Creates a new SigninPromoController.
     * @param profileDataCache The profile data cache for the latest used account on the device.
     * @param accessPoint Specifies the AccessPoint from which the promo is to be shown.
     */
    public SigninPromoController(
            ProfileDataCache profileDataCache, @AccountSigninActivity.AccessPoint int accessPoint) {
        mProfileDataCache = profileDataCache;
        mAccessPoint = accessPoint;

        switch (mAccessPoint) {
            case SigninAccessPoint.BOOKMARK_MANAGER:
                mImpressionCountName = SIGNIN_PROMO_IMPRESSIONS_COUNT_BOOKMARKS;
                mImpressionUserActionName = "Signin_Impression_FromBookmarkManager";
                mImpressionWithAccountUserActionName =
                        "Signin_ImpressionWithAccount_FromBookmarkManager";
                mImpressionWithNoAccountUserActionName =
                        "Signin_ImpressionWithNoAccount_FromBookmarkManager";
                mDescriptionStringId = R.string.signin_promo_description_bookmarks;
                break;
            case SigninAccessPoint.NTP_CONTENT_SUGGESTIONS:
                // There is no impression limit for NTP content suggestions.
                mImpressionCountName = null;
                mImpressionUserActionName = "Signin_Impression_FromNTPContentSuggestions";
                // TODO(iuliah): create Signin_ImpressionWithAccount_FromNTPContentSuggestions.
                mImpressionWithAccountUserActionName = null;
                // TODO(iuliah): create Signin_ImpressionWithNoAccount_FromNTPContentSuggestions.
                mImpressionWithNoAccountUserActionName = null;
                mDescriptionStringId = R.string.signin_promo_description_ntp_content_suggestions;
                break;
            case SigninAccessPoint.RECENT_TABS:
                // There is no impression limit for Recent Tabs.
                mImpressionCountName = null;
                mImpressionUserActionName = "Signin_Impression_FromRecentTabs";
                mImpressionWithAccountUserActionName =
                        "Signin_ImpressionWithAccount_FromRecentTabs";
                mImpressionWithNoAccountUserActionName =
                        "Signin_ImpressionWithNoAccount_FromRecentTabs";
                mDescriptionStringId = R.string.signin_promo_description_recent_tabs;
                break;
            case SigninAccessPoint.SETTINGS:
                mImpressionCountName = SIGNIN_PROMO_IMPRESSIONS_COUNT_SETTINGS;
                mImpressionUserActionName = "Signin_Impression_FromSettings";
                mImpressionWithAccountUserActionName = "Signin_ImpressionWithAccount_FromSettings";
                mImpressionWithNoAccountUserActionName =
                        "Signin_ImpressionWithNoAccount_FromSettings";
                mDescriptionStringId = R.string.signin_promo_description_settings;
                break;
            default:
                throw new IllegalArgumentException(
                        "Unexpected value for access point: " + mAccessPoint);
        }
    }

    /**
     * Records user actions for promo impressions.
     */
    public void recordSigninPromoImpression() {
        RecordUserAction.record(mImpressionUserActionName);
        if (mAccountName == null) {
            recordSigninImpressionWithNoAccountUserAction();
        } else {
            recordSigninImpressionWithAccountUserAction();
        }

        // If mImpressionCountName is not null then we should record impressions.
        if (mImpressionCountName != null) {
            SharedPreferences preferences = ContextUtils.getAppSharedPreferences();
            int numImpressions = preferences.getInt(mImpressionCountName, 0) + 1;

            // Whenever the promo is shown for the first time (i.e. a new impression), we should
            // reset the flag which keeps track of whether the user interacted with the promo.
            preferences.edit()
                    .putInt(mImpressionCountName, numImpressions)
                    .putBoolean(SIGNIN_PROMO_WAS_USED, false)
                    .apply();
        }
    }

    /**
     * Configures the signin promo view.
     * @param view The view in which the promo will be added.
     * @param onDismissListener Listener which handles the action of dismissing the promo. A null
     *         onDismissListener marks that the promo is not dismissible and as a result the close
     *         button is hidden.
     */
    public void setupSigninPromoView(Context context, SigninPromoView view,
            @Nullable final OnDismissListener onDismissListener) {
        view.getDescription().setText(mDescriptionStringId);

        if (mAccountName == null) {
            setupColdState(context, view);
        } else {
            setupHotState(context, view);
        }

        if (onDismissListener != null) {
            view.getDismissButton().setVisibility(View.VISIBLE);
            view.getDismissButton().setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    assert mAccessPoint == SigninAccessPoint.BOOKMARK_MANAGER;

                    setWasUsed(true);
                    int numImpressions = getNumImpressionsForAccessPoint();
                    RecordHistogram.recordCount100Histogram(
                            "MobileSignInPromo.BookmarkManager.CountTilXButton", numImpressions);
                    onDismissListener.onDismiss();
                }
            });
        } else {
            view.getDismissButton().setVisibility(View.GONE);
        }
    }

    /**
     * Sets the the default account found on the device.
     * @param accountName The name of the default account found on the device. Can be null if there
     *         are no accounts signed in on the device.
     */
    public void setAccountName(@Nullable String accountName) {
        mAccountName = accountName;
    }

    /**
     * Records MobileSignInPromo.[AccessPoint].ImpressionsTilDismiss histogram (i.e. whether the
     * user has not interacted with the promo whatsoever on the Xth impression).
     */
    public void recordImpressionsTilDismissHistogramIfNotUsed() {
        if (wasUsed() || mImpressionCountName == null) {
            return;
        }

        assert mAccessPoint == SigninAccessPoint.BOOKMARK_MANAGER
                || mAccessPoint == SigninAccessPoint.SETTINGS;

        final String histogram;
        if (mAccessPoint == SigninAccessPoint.BOOKMARK_MANAGER) {
            histogram = "MobileSignInPromo.BookmarkManager.ImpressionsTilDismiss";
        } else {
            histogram = "MobileSignInPromo.SettingsManager.ImpressionsTilDismiss";
        }

        RecordHistogram.recordCount100Histogram(histogram, getNumImpressionsForAccessPoint());
    }

    private void recordSigninImpressionWithAccountUserAction() {
        if (mImpressionWithAccountUserActionName != null) {
            RecordUserAction.record(mImpressionWithAccountUserActionName);
        }
    }

    private void recordSigninImpressionWithNoAccountUserAction() {
        if (mImpressionWithNoAccountUserActionName != null) {
            RecordUserAction.record(mImpressionWithNoAccountUserActionName);
        }
    }

    private void setupColdState(final Context context, SigninPromoView view) {
        view.getImage().setImageResource(R.drawable.chrome_sync_logo);
        setImageSize(context, view, R.dimen.signin_promo_cold_state_image_size);

        view.getSigninButton().setText(R.string.sign_in_to_chrome);
        view.getSigninButton().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                markButtonClicked();
                AccountSigninActivity.startFromAddAccountPage(context, mAccessPoint, true);
            }
        });

        view.getChooseAccountButton().setVisibility(View.GONE);
    }

    private void setupHotState(final Context context, SigninPromoView view) {
        Drawable accountImage = mProfileDataCache.getImage(mAccountName);
        view.getImage().setImageDrawable(accountImage);
        setImageSize(context, view, R.dimen.signin_promo_account_image_size);

        String accountTitle = getAccountTitle();
        String signinButtonText =
                context.getString(R.string.signin_promo_continue_as, accountTitle);
        view.getSigninButton().setText(signinButtonText);
        view.getSigninButton().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                markButtonClicked();
                AccountSigninActivity.startFromConfirmationPage(
                        context, mAccessPoint, mAccountName, true, true);
            }
        });

        String chooseAccountButtonText =
                context.getString(R.string.signin_promo_choose_account, mAccountName);
        view.getChooseAccountButton().setText(chooseAccountButtonText);
        view.getChooseAccountButton().setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                markButtonClicked();
                AccountSigninActivity.startAccountSigninActivity(context, mAccessPoint, true);
            }
        });
        view.getChooseAccountButton().setVisibility(View.VISIBLE);
    }

    /**
     * Stores the fact that the user has interacted with the promo by using on one of the buttons
     * and records the histograms which count how many times the signin buttons have been used on
     * the Xth impression.
     */
    private void markButtonClicked() {
        setWasUsed(true);

        // If mImpressionCountName is null then impressions aren't being recorded.
        if (mImpressionCountName == null) {
            return;
        }

        assert mAccessPoint == SigninAccessPoint.BOOKMARK_MANAGER
                || mAccessPoint == SigninAccessPoint.SETTINGS;

        final String histogram;
        if (mAccessPoint == SigninAccessPoint.BOOKMARK_MANAGER) {
            histogram = "MobileSignInPromo.BookmarkManager.CountTilSigninButtons";
        } else {
            histogram = "MobileSignInPromo.SettingsManager.CountTilSigninButtons";
        }

        RecordHistogram.recordCount100Histogram(histogram, getNumImpressionsForAccessPoint());
    }

    private void setWasUsed(boolean wasUsed) {
        SharedPreferences preferences = ContextUtils.getAppSharedPreferences();
        preferences.edit().putBoolean(SIGNIN_PROMO_WAS_USED, wasUsed).apply();
    }

    private boolean wasUsed() {
        SharedPreferences preferences = ContextUtils.getAppSharedPreferences();
        return preferences.getBoolean(SIGNIN_PROMO_WAS_USED, false);
    }

    private int getNumImpressionsForAccessPoint() {
        SharedPreferences preferences = ContextUtils.getAppSharedPreferences();
        return preferences.getInt(mImpressionCountName, 0);
    }

    private void setImageSize(Context context, SigninPromoView view, @DimenRes int dimenResId) {
        ViewGroup.LayoutParams layoutParams = view.getImage().getLayoutParams();
        layoutParams.height = context.getResources().getDimensionPixelSize(dimenResId);
        layoutParams.width = context.getResources().getDimensionPixelSize(dimenResId);
        view.getImage().setLayoutParams(layoutParams);
    }

    private String getAccountTitle() {
        String title = mProfileDataCache.getFullName(mAccountName);
        return title == null ? mAccountName : title;
    }
}
