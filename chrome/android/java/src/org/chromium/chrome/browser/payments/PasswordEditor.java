// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import android.app.ProgressDialog;
import android.text.TextUtils;

import org.chromium.base.Callback;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.SavedPasswordEntry;
import org.chromium.chrome.browser.preferences.password.PasswordManagerHandlerProvider;
import org.chromium.chrome.browser.widget.prefeditor.EditorBase;
import org.chromium.chrome.browser.widget.prefeditor.EditorFieldModel;
import org.chromium.chrome.browser.widget.prefeditor.EditorFieldModel.EditorFieldValidator;
import org.chromium.chrome.browser.widget.prefeditor.EditorModel;
import org.chromium.net.GURLUtils;

import javax.annotation.Nullable;

/**
 * A password editor. Will be used for editing already saved passwords or for creating new
 * entries. In order to create new password entry user need to type URL with the valid
 * origin, username and password.
 */
public class PasswordEditor extends EditorBase<SavedPasswordEntry> {
    private EditorFieldModel mUsername;
    private EditorFieldModel mSite;
    private EditorFieldModel mPassword;
    private ProgressDialog mProgressDialog;
    private EditorModel mEditor;
    final private SiteValidator mSiteValidator;

    public PasswordEditor() {
        mSiteValidator = new SiteValidator();
    }

    @Override
    public void edit(@Nullable final SavedPasswordEntry toEdit,
            final Callback<SavedPasswordEntry> callback) {
        // toEdit is ignored for now. Only adding new credentials is supported.
        super.edit(null /* toEdit */, callback);

        final SavedPasswordEntry credential = new SavedPasswordEntry("", "", "", "");
        mEditor = new EditorModel(mContext.getString(R.string.password_entry_editor_add));

        mSite = EditorFieldModel.createTextInput(EditorFieldModel.INPUT_TYPE_HINT_URL,
                mContext.getString(R.string.password_entry_editor_site_title), null, null,
                mSiteValidator, null,
                mContext.getString(R.string.pref_edit_dialog_field_required_validation_message),
                mContext.getString(R.string.site_invalid_validation_message), null);

        mUsername = EditorFieldModel.createTextInput();
        mUsername.setLabel(mContext.getString(R.string.password_entry_editor_username_title));
        mUsername.setRequiredErrorMessage(
                mContext.getString(R.string.pref_edit_dialog_field_required_validation_message));

        mPassword = EditorFieldModel.createTextInput();
        mPassword.setLabel(mContext.getString(R.string.password_entry_editor_password));
        mPassword.setRequiredErrorMessage(
                mContext.getString(R.string.pref_edit_dialog_field_required_validation_message));

        mEditor.addField(mSite);
        mEditor.addField(mUsername);
        mEditor.addField(mPassword);

        mEditor.setCancelCallback(() -> {});

        mEditor.setDoneCallback(() -> {
            commitChanges();
            callback.onResult(credential);
        });

        mEditorDialog.show(mEditor);
    }

    private void showProgressDialog() {
        mProgressDialog = new ProgressDialog(mContext);
        mProgressDialog.setMessage(mContext.getText(R.string.payments_loading_message));
        mProgressDialog.show();
    }

    private void dismissProgressDialog() {
        if (mProgressDialog != null && mProgressDialog.isShowing()) {
            mProgressDialog.dismiss();
        }
        mProgressDialog = null;
    }

    private void commitChanges() {
        PasswordManagerHandlerProvider.getInstance().getPasswordManagerHandler().addPasswordEntry(
                mSite.getValue().toString(), mUsername.getValue().toString(),
                mPassword.getValue().toString(), GURLUtils.getOrigin(mSite.getValue().toString()));
    }

    private static class SiteValidator implements EditorFieldValidator {
        @Override
        public boolean isValid(@Nullable CharSequence value) {
            return value != null && !TextUtils.isEmpty(GURLUtils.getOrigin(value.toString()));
        }

        @Override
        public boolean isLengthMaximum(@Nullable CharSequence value) {
            return false;
        }
    }
}
