// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.bookmarks;

import static android.support.test.espresso.Espresso.onView;
import static android.support.test.espresso.Espresso.pressBack;
import static android.support.test.espresso.action.ViewActions.click;
import static android.support.test.espresso.assertion.ViewAssertions.doesNotExist;
import static android.support.test.espresso.assertion.ViewAssertions.matches;
import static android.support.test.espresso.matcher.ViewMatchers.isDisplayed;
import static android.support.test.espresso.matcher.ViewMatchers.withId;
import static android.support.test.espresso.matcher.ViewMatchers.withText;
import static android.view.View.GONE;
import static android.view.View.VISIBLE;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertTrue;

import android.accounts.Account;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.support.test.espresso.ViewInteraction;
import android.support.test.filters.LargeTest;
import android.support.test.runner.intent.IntentCallback;
import android.support.test.runner.intent.IntentMonitorRegistry;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.signin.AccountSigninActivity;
import org.chromium.chrome.browser.signin.PersonalizedSigninPromoView;
import org.chromium.chrome.browser.signin.SigninAccessPoint;
import org.chromium.chrome.browser.signin.SigninPromoController;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ProfileDataSource;
import org.chromium.components.signin.test.util.AccountHolder;
import org.chromium.components.signin.test.util.FakeAccountManagerDelegate;

import java.io.Closeable;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Tests for the personalized signin promo on the Bookmarks page.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        "enable-features=AndroidSigninPromos"})
public class BookmarkPersonalizedSigninPromoTest {
    private static final String TEST_ACCOUNT_NAME = "test@gmail.com";
    private static final String TEST_PASSWORD = "password";
    private static final String TEST_FULL_NAME = "Test Account";
    private static final String TEST_GIVEN_NAME = "Test";

    @Rule
    public final ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private FakeAccountManagerDelegate mAccountManagerDelegate;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityFromLauncher();
        mAccountManagerDelegate = new FakeAccountManagerDelegate(
                FakeAccountManagerDelegate.ENABLE_PROFILE_DATA_SOURCE);
        AccountManagerFacade.overrideAccountManagerFacadeForTests(mAccountManagerDelegate);
    }

    @Test
    @LargeTest
    public void testManualDismissPromo() throws InterruptedException {
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));
        onView(withId(R.id.signin_promo_close_button)).perform(click());
        onView(withId(R.id.signin_promo_view_container)).check(doesNotExist());
    }

    @Test
    @LargeTest
    public void testAutoDismissPromo() throws InterruptedException {
        int impressionCap = SigninPromoController.getMaxImpressionsBookmarksForTests();
        for (int impression = 0; impression < impressionCap; impression++) {
            openBookmarkManager();
            onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));
            pressBack();
        }
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(doesNotExist());
    }

    @Test
    @LargeTest
    public void testColdState() throws InterruptedException {
        openBookmarkManager();
        ViewInteraction signinPromoContainer = onView(withId(R.id.signin_promo_view_container));
        signinPromoContainer.check(matches(isDisplayed()));
        signinPromoContainer.check((view, e) -> {
            PersonalizedSigninPromoView promoView = (PersonalizedSigninPromoView) view;

            assertEquals(VISIBLE, promoView.getDismissButton().getVisibility());

            Drawable.ConstantState expectedPromoImage =
                    mActivityTestRule.getActivity()
                            .getResources()
                            .getDrawable(R.drawable.chrome_sync_logo)
                            .getConstantState();
            assertEquals(expectedPromoImage, promoView.getImage().getDrawable().getConstantState());

            String expectedPromoDescription =
                    mActivityTestRule.getActivity().getResources().getString(
                            R.string.signin_promo_description_bookmarks);
            assertEquals(expectedPromoDescription, promoView.getDescription().getText());

            String expectedSigninButtonText =
                    mActivityTestRule.getActivity().getResources().getString(
                            R.string.sign_in_to_chrome);
            assertEquals(expectedSigninButtonText, promoView.getSigninButton().getText());
            assertEquals(VISIBLE, promoView.getSigninButton().getVisibility());

            assertEquals(GONE, promoView.getChooseAccountButton().getVisibility());
        });
    }

    @Test
    @LargeTest
    public void testHotState() throws InterruptedException {
        addTestAccount();
        openBookmarkManager();
        ViewInteraction signinPromoContainer = onView(withId(R.id.signin_promo_view_container));
        signinPromoContainer.check(matches(isDisplayed()));
        signinPromoContainer.check((view, e) -> {
            PersonalizedSigninPromoView promoView = (PersonalizedSigninPromoView) view;

            assertEquals(VISIBLE, promoView.getDismissButton().getVisibility());

            Drawable.ConstantState expectedPromoImage =
                    mActivityTestRule.getActivity()
                            .getResources()
                            .getDrawable(R.drawable.logo_avatar_anonymous)
                            .getConstantState();
            assertEquals(expectedPromoImage, promoView.getImage().getDrawable().getConstantState());

            String expectedPromoDescription =
                    mActivityTestRule.getActivity().getResources().getString(
                            R.string.signin_promo_description_bookmarks);
            assertEquals(expectedPromoDescription, promoView.getDescription().getText());

            String expectedSigninButtonText =
                    mActivityTestRule.getActivity().getResources().getString(
                            R.string.signin_promo_continue_as, TEST_FULL_NAME);
            assertEquals(expectedSigninButtonText, promoView.getSigninButton().getText());
            assertEquals(VISIBLE, promoView.getSigninButton().getVisibility());

            String expectedChooseAccountText =
                    mActivityTestRule.getActivity().getResources().getString(
                            R.string.signin_promo_choose_account, TEST_ACCOUNT_NAME);
            assertEquals(expectedChooseAccountText, promoView.getChooseAccountButton().getText());
            assertEquals(VISIBLE, promoView.getChooseAccountButton().getVisibility());
        });
    }

    @Test
    @LargeTest
    public void testSigninButtonDefaultAccount() throws Exception {
        addTestAccount();
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));

        final List<Intent> startedIntents;
        try (IntentCallbackHelper helper = new IntentCallbackHelper()) {
            onView(withId(R.id.signin_promo_signin_button)).perform(click());
            startedIntents = helper.getStartedIntents();
        }

        assertEquals("Choosing to sign in with the default account should fire an intent!", 1,
                startedIntents.size());
        Intent expectedIntent = AccountSigninActivity.createIntentForConfirmationOnlySigninFlow(
                mActivityTestRule.getActivity(), SigninAccessPoint.BOOKMARK_MANAGER,
                TEST_ACCOUNT_NAME, true, true);
        Intent startedIntent = startedIntents.get(0);
        assertTrue(expectedIntent.filterEquals(startedIntent));
    }

    @Test
    @LargeTest
    public void testSigninButtonNotDefaultAccount() throws Exception {
        addTestAccount();
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));

        final List<Intent> startedIntents;
        try (IntentCallbackHelper helper = new IntentCallbackHelper()) {
            onView(withId(R.id.signin_promo_choose_account_button)).perform(click());
            startedIntents = helper.getStartedIntents();
        }

        assertEquals("Choosing to sign in with another account should fire an intent!", 1,
                startedIntents.size());
        Intent expectedIntent = AccountSigninActivity.createIntentForDefaultSigninFlow(
                mActivityTestRule.getActivity(), SigninAccessPoint.BOOKMARK_MANAGER, true);
        Intent startedIntent = startedIntents.get(0);
        assertTrue(expectedIntent.filterEquals(startedIntent));
    }

    @Test
    @LargeTest
    public void testSigninButtonNewAccount() throws Exception {
        openBookmarkManager();
        onView(withId(R.id.signin_promo_view_container)).check(matches(isDisplayed()));

        final List<Intent> startedIntents;
        try (IntentCallbackHelper helper = new IntentCallbackHelper()) {
            onView(withId(R.id.signin_promo_signin_button)).perform(click());
            startedIntents = helper.getStartedIntents();
        }

        assertEquals("Adding a new account should fire 2 intents!", 2, startedIntents.size());
        Intent expectedIntent = AccountSigninActivity.createIntentForAddAccountSigninFlow(
                mActivityTestRule.getActivity(), SigninAccessPoint.BOOKMARK_MANAGER, true);
        // Comparing only the first intent as AccountSigninActivity will fire an intent after
        // starting the flow to add an account.
        Intent firstStartedIntent = startedIntents.get(0);
        assertTrue(expectedIntent.filterEquals(firstStartedIntent));
    }

    private void openBookmarkManager() throws InterruptedException {
        onView(withId(R.id.menu_button)).perform(click());
        onView(withText("Bookmarks")).perform(click());
    }

    private void addTestAccount() {
        Account account = AccountManagerFacade.createAccountFromName(TEST_ACCOUNT_NAME);
        AccountHolder.Builder accountHolder =
                AccountHolder.builder(account).password(TEST_PASSWORD).alwaysAccept(true);
        mAccountManagerDelegate.addAccountHolderExplicitly(accountHolder.build());
        Context context = mActivityTestRule.getActivity();
        Bitmap placeHolderImage = BitmapFactory.decodeResource(
                context.getResources(), R.drawable.logo_avatar_anonymous);
        ProfileDataSource.ProfileData profileData = new ProfileDataSource.ProfileData(
                TEST_ACCOUNT_NAME, placeHolderImage, TEST_FULL_NAME, TEST_GIVEN_NAME);
        ThreadUtils.runOnUiThreadBlocking(
                () -> mAccountManagerDelegate.setProfileData(TEST_ACCOUNT_NAME, profileData));
    }

    private class IntentCallbackHelper implements IntentCallback, Closeable {
        private final List<Intent> mStartedIntents = new ArrayList<>();

        IntentCallbackHelper() {
            IntentMonitorRegistry.getInstance().addIntentCallback(this);
        }

        @Override
        public void onIntentSent(Intent intent) {
            mStartedIntents.add(intent);
        }

        @Override
        public void close() throws IOException {
            IntentMonitorRegistry.getInstance().removeIntentCallback(this);
        }

        public List<Intent> getStartedIntents() {
            return mStartedIntents;
        }
    }
}
