// Copyright 2017 The Chromium Authors. All rights reserved.
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
    /** Context for the app. */
    protected Activity mActivity;
    /** GUID of the profile we are editing. Empty if creating a new profile. */
    private String mGUID;

    public AutofillProfileEditorPreference(Activity context) {
        super(context);
        mActivity = context;
    }

    @Override
    protected void onClick() {
        AutofillAddress autofillAddress = null;
        Runnable runnable = null;
        Bundle extras = getExtras();
        // We know which profile to edit based on the GUID stuffed in our extras
        // by AutofillAndPaymentsPreferences.
        mGUID = extras.getString(AutofillAndPaymentsPreferences.AUTOFILL_GUID);
        if (mGUID != null) {
            autofillAddress = new AutofillAddress(
                    mActivity, PersonalDataManager.getInstance().getProfile(mGUID));
            runnable = new Runnable() {
                @Override
                public void run() {
                    if (mGUID != null) {
                        PersonalDataManager.getInstance().deleteProfile(mGUID);
                    }
                }
            };
        }

        EditorDialog editorDialog = new EditorDialog(mActivity, null, runnable);
        AddressEditor addressEditor = new AddressEditor(/*emailIncluded=*/true);
        addressEditor.setEditorDialog(editorDialog);

        addressEditor.edit(autofillAddress, new Callback<AutofillAddress>() {
            @Override
            public void onResult(AutofillAddress address) {
                if (address != null) {
                    PersonalDataManager.getInstance().setProfile(address.getProfile());
                }
            }
        });
    }
}
