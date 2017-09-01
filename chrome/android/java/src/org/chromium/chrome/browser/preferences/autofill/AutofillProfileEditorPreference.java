// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.app.Activity;
import android.os.Bundle;
import android.preference.Preference;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.payments.AddressEditor;
import org.chromium.chrome.browser.payments.AutofillAddress;
import org.chromium.chrome.browser.payments.ui.EditorDialog;

/**
 * Provides the Java-ui for editing a Profile autofill entry.
 */
public class AutofillProfileEditorPreference extends Preference {
    private AddressEditor mAddressEditor;

    /** Context for the app. */
    protected Activity mActivity;
    private AutofillAddress mAutofillAddress;
    private String mGUID;
    EditorDialog mEditorDialog;

    public AutofillProfileEditorPreference(Activity context) {
        super(context);
        mActivity = context;
    }

    @Override
    protected void onClick() {
        Runnable runnable = null;
        Bundle extras = getExtras();
        // guid of the profile we are editing. Empty if creating a new profile.
        mGUID = extras.getString(AutofillAndPaymentsPreferences.AUTOFILL_GUID);
        // We know which profile to edit based on the guid stuffed in
        // our extras by AutofillAndPaymentsPreferences.
        if (mGUID == null) {
            mGUID = "";
            mAutofillAddress = null;
        } else {
            mAutofillAddress = new AutofillAddress(
                    mActivity, PersonalDataManager.getInstance().getProfile(mGUID));
            runnable = new Runnable() {
                @Override
                public void run() {
                    if (mGUID != null) {
                        PersonalDataManager.getInstance().deleteProfile(mGUID);
                    }
                }
                // mActivity.finish(); /// ?
            };
        }

        mEditorDialog = new EditorDialog(mActivity, null, runnable);
        mAddressEditor = new AddressEditor();
        mAddressEditor.setEditorDialog(mEditorDialog);

        mAddressEditor.edit(mAutofillAddress, new Callback<AutofillAddress>() {
            @Override
            public void onResult(AutofillAddress address) {
                if (address != null) {
                    PersonalDataManager.getInstance().setProfile(address.getProfile());
                }
            }
        });
    }
}
