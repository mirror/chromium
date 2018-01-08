// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.signin.test;

import static org.robolectric.Shadows.shadowOf;

import android.accounts.Account;
import android.content.Context;
import android.os.Bundle;
import android.os.UserManager;
import android.support.test.filters.SmallTest;
import android.support.test.rule.UiThreadTestRule;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.base.ContextUtils;
import org.chromium.components.signin.AccountManagerDelegateException;
import org.chromium.components.signin.AccountManagerFacade;
import org.chromium.components.signin.ProfileDataSource;
import org.chromium.components.signin.test.util.AccountHolder;
import org.chromium.components.signin.test.util.FakeAccountManagerDelegate;
import org.chromium.testing.local.CustomShadowAsyncTask;
import org.chromium.testing.local.CustomShadowUserManager;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Test class for {@link AccountManagerFacade}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE,
        shadows = {CustomShadowAsyncTask.class, CustomShadowUserManager.class})
public class AccountManagerFacadeTest {
    @Rule
    public UiThreadTestRule mRule = new UiThreadTestRule();

    private CustomShadowUserManager mShadowUserManager;
    private FakeAccountManagerDelegate mDelegate;
    private AccountManagerFacade mFacade;

    @Before
    public void setUp() throws Exception {
        Context context = RuntimeEnvironment.application;
        ContextUtils.initApplicationContextForTests(context);
        UserManager userManager = (UserManager) context.getSystemService(Context.USER_SERVICE);
        mShadowUserManager = (CustomShadowUserManager) shadowOf(userManager);
    }

    private void initAccountManagerFacade() {
        Assert.assertNull(mDelegate);
        mDelegate = new FakeAccountManagerDelegate(
                FakeAccountManagerDelegate.ENABLE_PROFILE_DATA_SOURCE);
        Assert.assertFalse(mDelegate.isRegisterObserversCalled());
        AccountManagerFacade.overrideAccountManagerFacadeForTests(mDelegate);
        Assert.assertTrue(mDelegate.isRegisterObserversCalled());
        mFacade = AccountManagerFacade.get();
    }

    private void initAccountManagerFacadeWithAccountRestrictionPattern(String pattern) {
        Bundle restrictions = new Bundle();
        restrictions.putString(AccountManagerFacade.ACCOUNT_RESTRICTION_PATTERN_KEY, pattern);
        mShadowUserManager.setApplicationRestrictions(
                RuntimeEnvironment.application.getPackageName(), restrictions);
        initAccountManagerFacade();
    }

    @Test
    @SmallTest
    public void testCanonicalAccount() {
        initAccountManagerFacade();
        addTestAccount("test@gmail.com");

        Assert.assertTrue(mFacade.hasAccountForName("test@gmail.com"));
        Assert.assertTrue(mFacade.hasAccountForName("Test@gmail.com"));
        Assert.assertTrue(mFacade.hasAccountForName("te.st@gmail.com"));
    }

    // If this test starts flaking, please re-open crbug.com/568636 and make sure there is some sort
    // of stack trace or error message in that bug BEFORE disabling the test.
    @Test
    @SmallTest
    public void testNonCanonicalAccount() {
        initAccountManagerFacade();
        addTestAccount("test.me@gmail.com");

        Assert.assertTrue(mFacade.hasAccountForName("test.me@gmail.com"));
        Assert.assertTrue(mFacade.hasAccountForName("testme@gmail.com"));
        Assert.assertTrue(mFacade.hasAccountForName("Testme@gmail.com"));
        Assert.assertTrue(mFacade.hasAccountForName("te.st.me@gmail.com"));
    }

    @Test
    @SmallTest
    public void testProfileDataSource() throws Throwable {
        initAccountManagerFacade();
        String accountName = "test@gmail.com";
        addTestAccount(accountName);

        mRule.runOnUiThread(() -> {
            ProfileDataSource.ProfileData profileData = new ProfileDataSource.ProfileData(
                    accountName, null, "Test Full Name", "Test Given Name");

            ProfileDataSource profileDataSource = mDelegate.getProfileDataSource();
            Assert.assertNotNull(profileDataSource);
            mDelegate.setProfileData(accountName, profileData);
            Assert.assertArrayEquals(profileDataSource.getProfileDataMap().values().toArray(),
                    new ProfileDataSource.ProfileData[] {profileData});

            mDelegate.setProfileData(accountName, null);
            Assert.assertArrayEquals(profileDataSource.getProfileDataMap().values().toArray(),
                    new ProfileDataSource.ProfileData[0]);
        });
    }

    @Test
    @SmallTest
    public void testGetAccounts() throws AccountManagerDelegateException {
        initAccountManagerFacade();
        Assert.assertArrayEquals(new Account[] {}, mFacade.getGoogleAccounts());

        Account account = addTestAccount("test@gmail.com");
        Assert.assertArrayEquals(new Account[] {account}, mFacade.getGoogleAccounts());

        Account account2 = addTestAccount("test2@gmail.com");
        Assert.assertArrayEquals(new Account[] {account, account2}, mFacade.getGoogleAccounts());

        Account account3 = addTestAccount("test3@gmail.com");
        Assert.assertArrayEquals(
                new Account[] {account, account2, account3}, mFacade.getGoogleAccounts());

        removeTestAccount(account2);
        Assert.assertArrayEquals(new Account[] {account, account3}, mFacade.getGoogleAccounts());
    }

    @Test
    @SmallTest
    public void testGetAccountsWithRestrictionPattern() throws AccountManagerDelegateException {
        initAccountManagerFacadeWithAccountRestrictionPattern(".*@example\\.com$");
        Account account = addTestAccount("test@example.com");
        Assert.assertArrayEquals(new Account[] {account}, mFacade.getGoogleAccounts());

        // Add account that doesn't match the pattern.
        addTestAccount("test@gmail.com");
        Assert.assertArrayEquals(new Account[] {account}, mFacade.getGoogleAccounts());

        Account account2 = addTestAccount("test2@example.com");
        Assert.assertArrayEquals(new Account[] {account, account2}, mFacade.getGoogleAccounts());

        // Add another account that doesn't match the pattern.
        addTestAccount("test2@gmail.com");
        Assert.assertArrayEquals(new Account[] {account, account2}, mFacade.getGoogleAccounts());

        removeTestAccount(account);
        Assert.assertArrayEquals(new Account[] {account2}, mFacade.getGoogleAccounts());
    }

    private Account addTestAccount(String accountName) {
        Account account = AccountManagerFacade.createAccountFromName(accountName);
        AccountHolder holder = AccountHolder.builder(account).alwaysAccept(true).build();
        mDelegate.addAccountHolderExplicitly(holder);
        Assert.assertFalse(AccountManagerFacade.get().isUpdatePending());
        return account;
    }

    private void removeTestAccount(Account account) {
        mDelegate.removeAccountHolderExplicitly(AccountHolder.builder(account).build());
    }
}
