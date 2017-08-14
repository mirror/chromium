// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.app.Activity;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.support.v7.app.AlertDialog;
import android.text.Editable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.banners.AppBannerManager;

/**
 * Displays the "Add to Homescreen" dialog.
 */
public class AddToHomescreenDialog implements AddToHomescreenManager.Observer {
    private AlertDialog mDialog;
    private View mProgressBarView;
    private ImageView mIconView;
    private TextView mInput;
    private View mView;

    private AddToHomescreenManager mManager;

    /**
     * Whether {@link mManager} is ready for {@link AddToHomescreenManager#addShortcut()} to be
     * called.
     */
    private boolean mIsReadyToAdd;

    public AddToHomescreenDialog(AddToHomescreenManager manager) {
        mManager = manager;
    }

    @VisibleForTesting
    public AlertDialog getAlertDialogForTesting() {
        return mDialog;
    }

    /**
     * Shows the dialog for adding a shortcut to the home screen.
     * @param activity The current activity in which to create the dialog.
     */
    public void show(final Activity activity) {
        mView = activity.getLayoutInflater().inflate(R.layout.add_to_homescreen_dialog, null);
        AlertDialog.Builder builder = new AlertDialog.Builder(activity, R.style.AlertDialogTheme)
                .setTitle(AppBannerManager.getHomescreenLanguageOption())
                .setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int id) {
                                dialog.cancel();
                            }
                        });

        mDialog = builder.create();
        mDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        // On click of the menu item for "add to homescreen", an alert dialog pops asking the user
        // if the title needs to be edited. On click of "Add", shortcut is created. Default
        // title is the title of the page.
        mProgressBarView = mView.findViewById(R.id.spinny);
        mIconView = (ImageView) mView.findViewById(R.id.icon);
        mInput = (EditText) mView.findViewById(R.id.text);

        // The dialog's text field is disabled till the "user title" is fetched,
        mInput.setEnabled(false);
        mInput.setVisibility(mView.INVISIBLE);

        mView.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                if (mProgressBarView.getMeasuredHeight() == mInput.getMeasuredHeight()
                        && mInput.getBackground() != null) {
                    // Force the text field to align better with the icon by accounting for the
                    // padding introduced by the background drawable.
                    mInput.getLayoutParams().height =
                            mProgressBarView.getMeasuredHeight() + mInput.getPaddingBottom();
                    v.requestLayout();
                    v.removeOnLayoutChangeListener(this);
                }
            }
        });

        // The "Add" button should be disabled if the dialog's text field is empty.
        mInput.addTextChangedListener(new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void afterTextChanged(Editable editableText) {
                updateAddButtonEnabledState();
            }
        });

        mDialog.setView(mView);
        mDialog.setButton(DialogInterface.BUTTON_POSITIVE,
                activity.getResources().getString(R.string.add),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id) {
                        mManager.addShortcut(mInput.getText().toString());
                    }
                });

        mDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface d) {
                updateAddButtonEnabledState();
            }
        });

        mDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                mDialog = null;
                mManager.destroy();
            }
        });

        mDialog.show();
    }

    /**
     * Called when the Web Manifest of the page is available.
     */
    @Override
    public void onUserTitleAvailable(
            String title, String name, String origin, boolean isTitleEditable) {
        if (isTitleEditable) {
            mInput.setEnabled(isTitleEditable);
            mInput.setText(title);
            mInput.setVisibility(View.VISIBLE);
            return;
        }

        mInput = (TextView) mView.findViewById(R.id.webapk_text);
        SpannableString text = new SpannableString(
                TextUtils.concat(name, System.getProperty("line.separator"), origin));
        text.setSpan(new ForegroundColorSpan(Color.BLACK), 0, name.length(), 0); // set color
        mInput.setSingleLine(false);
        mInput.setText(text);
        mInput.setTextColor(Color.LTGRAY);
        mInput.setVisibility(View.VISIBLE);
    }

    /**
     * Called once the manager has finished fetching the homescreen shortcut's data (like the Web
     * Manifest) and is ready for {@link AddToHomescreenManager#addShortcut()} to be called.
     * @param icon Icon to use in the launcher.
     */
    @Override
    public void onReadyToAdd(Bitmap icon) {
        mIsReadyToAdd = true;

        mProgressBarView.setVisibility(View.GONE);
        mIconView.setVisibility(View.VISIBLE);
        mIconView.setImageBitmap(icon);
        updateAddButtonEnabledState();
    }

    /**
     * Updates whether the dialog's OK button is enabled.
     */
    public void updateAddButtonEnabledState() {
        boolean enable = mIsReadyToAdd && !TextUtils.isEmpty(mInput.getText());
        mDialog.getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(enable);
    }
}
