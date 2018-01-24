// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.password;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.view.View;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.chrome.R;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Tests for the "Save Passwords" settings screen.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ReauthenticationManagerTest {
    /**
     * Prepares a dummy Intent to pass to PasswordReauthenticationFragment as a fake result of the
     * reauthentication screen.
     * @return The dummy Intent.
     */
    private Intent prepareDummyDataForActivityResult() {
        Intent data = new Intent();
        data.putExtra("result", "This is the result");
        return data;
    }

    /**
     * Prepares a dummy Fragment and commits a FragmentTransaction with it.
     * @param fragmentManager To be used for creating the transaction.
     */
    private void addDummyPasswordEntryEditor(FragmentManager fragmentManager) {
        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
        // Replacement fragment for PasswordEntryEditor, which is the fragment that
        // replaces PasswordReauthentication after popBackStack is called.
        Fragment mockPasswordEntryEditor = new Fragment();
        fragmentTransaction.add(mockPasswordEntryEditor, "password_entry_editor");
        fragmentTransaction.addToBackStack(null);
        fragmentTransaction.commit();
    }

    /**
     * Ensure that displayReauthenticationFragment puts the reauthentication fragment on the
     * transaction stack and updates the validity of the reauth when reauth passed.
     */
    @Test
    public void testDisplayReauthenticationFragment_Passed() {
        Activity testActivity = Robolectric.setupActivity(Activity.class);
        PasswordReauthenticationFragment.preventLockingForTesting();

        FragmentManager fragmentManager = testActivity.getFragmentManager();
        addDummyPasswordEntryEditor(fragmentManager);

        ReauthenticationManager.resetLastReauth();
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME));
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.BULK));

        ReauthenticationManager.displayReauthenticationFragment(
                R.string.lockscreen_description_view, View.NO_ID, fragmentManager,
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME);
        Fragment reauthFragment =
                fragmentManager.findFragmentByTag(ReauthenticationManager.FRAGMENT_TAG);
        assertNotNull(reauthFragment);

        reauthFragment.onActivityResult(
                PasswordReauthenticationFragment.CONFIRM_DEVICE_CREDENTIAL_REQUEST_CODE,
                testActivity.RESULT_OK, prepareDummyDataForActivityResult());
        fragmentManager.executePendingTransactions();

        assertTrue(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME));
    }

    /**
     * Ensure that displayReauthenticationFragment puts the reauthentication fragment on the
     * transaction stack and updates the validity of the reauth when reauth failed.
     */
    @Test
    public void testDisplayReauthenticationFragment_Failed() {
        Activity testActivity = Robolectric.setupActivity(Activity.class);
        PasswordReauthenticationFragment.preventLockingForTesting();

        FragmentManager fragmentManager = testActivity.getFragmentManager();
        addDummyPasswordEntryEditor(fragmentManager);

        ReauthenticationManager.resetLastReauth();
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME));
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.BULK));

        ReauthenticationManager.displayReauthenticationFragment(
                R.string.lockscreen_description_view, View.NO_ID, fragmentManager,
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME);
        Fragment reauthFragment =
                fragmentManager.findFragmentByTag(ReauthenticationManager.FRAGMENT_TAG);
        assertNotNull(reauthFragment);

        reauthFragment.onActivityResult(
                PasswordReauthenticationFragment.CONFIRM_DEVICE_CREDENTIAL_REQUEST_CODE,
                testActivity.RESULT_CANCELED, prepareDummyDataForActivityResult());
        fragmentManager.executePendingTransactions();

        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME));
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.BULK));
    }

    /**
     * Ensure that displayReauthenticationFragment considers BULK scope to cover the ONE_AT_A_TIME
     * scope as well.
     */
    @Test
    public void testDisplayReauthenticationFragment_OneAtATimeCovered() {
        Activity testActivity = Robolectric.setupActivity(Activity.class);
        PasswordReauthenticationFragment.preventLockingForTesting();

        FragmentManager fragmentManager = testActivity.getFragmentManager();
        addDummyPasswordEntryEditor(fragmentManager);

        ReauthenticationManager.resetLastReauth();
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME));
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.BULK));

        ReauthenticationManager.displayReauthenticationFragment(
                R.string.lockscreen_description_export, View.NO_ID, fragmentManager,
                ReauthenticationManager.ReauthScope.BULK);
        Fragment reauthFragment =
                fragmentManager.findFragmentByTag(ReauthenticationManager.FRAGMENT_TAG);
        assertNotNull(reauthFragment);

        reauthFragment.onActivityResult(
                PasswordReauthenticationFragment.CONFIRM_DEVICE_CREDENTIAL_REQUEST_CODE,
                testActivity.RESULT_OK, prepareDummyDataForActivityResult());
        fragmentManager.executePendingTransactions();

        // Both BULK and ONE_AT_A_TIME scopes should be covered by the BULK request above.
        assertTrue(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME));
        assertTrue(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.BULK));
    }

    /**
     * Ensure that displayReauthenticationFragment does not consider ONE_AT_A_TIME scope to cover
     * the BULK scope.
     */
    @Test
    public void testDisplayReauthenticationFragment_BulkNotCovered() {
        Activity testActivity = Robolectric.setupActivity(Activity.class);
        PasswordReauthenticationFragment.preventLockingForTesting();

        FragmentManager fragmentManager = testActivity.getFragmentManager();
        addDummyPasswordEntryEditor(fragmentManager);

        ReauthenticationManager.resetLastReauth();
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME));
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.BULK));

        ReauthenticationManager.displayReauthenticationFragment(
                R.string.lockscreen_description_view, View.NO_ID, fragmentManager,
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME);
        Fragment reauthFragment =
                fragmentManager.findFragmentByTag(ReauthenticationManager.FRAGMENT_TAG);
        assertNotNull(reauthFragment);

        reauthFragment.onActivityResult(
                PasswordReauthenticationFragment.CONFIRM_DEVICE_CREDENTIAL_REQUEST_CODE,
                testActivity.RESULT_OK, prepareDummyDataForActivityResult());
        fragmentManager.executePendingTransactions();

        // Only ONE_AT_A_TIME scope should be covered by the ONE_AT_A_TIME request above.
        assertTrue(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.ONE_AT_A_TIME));
        assertFalse(ReauthenticationManager.authenticationStillValid(
                ReauthenticationManager.ReauthScope.BULK));
    }
}
