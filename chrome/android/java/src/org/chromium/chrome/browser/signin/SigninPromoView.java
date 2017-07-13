// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;

import org.chromium.chrome.R;
import org.chromium.ui.widget.ButtonCompat;

/**
 * Container view for signin promos
 */
public class SigninPromoView extends LinearLayout {
    private ImageView mImage;
    private ImageButton mDismissButton;
    private ButtonCompat mSigninButton;
    private Button mChooseButton;

    public SigninPromoView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mImage = (ImageView) findViewById(R.id.signin_promo_image);
        mDismissButton = (ImageButton) findViewById(R.id.signin_promo_close_button);
        mSigninButton = (ButtonCompat) findViewById(R.id.signin_promo_button_continue);
        mChooseButton = (Button) findViewById(R.id.signin_promo_button_choose_account);
    }

    public ImageView getImage() {
        return mImage;
    }

    public ImageButton getDismissButton() {
        return mDismissButton;
    }

    public ButtonCompat getSigninButton() {
        return mSigninButton;
    }

    public Button getChooseButton() {
        return mChooseButton;
    }
}
