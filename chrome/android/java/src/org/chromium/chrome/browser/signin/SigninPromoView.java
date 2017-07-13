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
    private ButtonCompat mSigninButton;
    private ImageView mPromoImage;
    private Button mChooseButton;
    private ImageButton mDismissButton;

    public SigninPromoView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mSigninButton = findViewById(R.id.signin_promo_button_continue);
        mChooseButton = findViewById(R.id.signin_promo_button_choose_account);
        mPromoImage = findViewById(R.id.signin_promo_image);
        mDismissButton = findViewById(R.id.signin_promo_close_button);
    }

    public ButtonCompat getSigninButton() {
        return mSigninButton;
    }

    public ImageView getPromoImage() {
        return mPromoImage;
    }

    public Button getChooseButton() {
        return mChooseButton;
    }

    public ImageButton getDismissButton() {
        return mDismissButton;
    }
}
