// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.modaldialog;

import android.content.Context;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.view.View;
import android.widget.FrameLayout;

import org.chromium.chrome.R;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Layout for ModalDialog's content view that contains a dialog view and a scrim view.
 */
public class ModalDialogLayout extends FrameLayout {
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({FULL_SCRIM, TOP_SCRIM, BOTTOM_SCRIM})
    public @interface ScrimState {}
    public static final int FULL_SCRIM = 0;
    public static final int TOP_SCRIM = 1;
    public static final int BOTTOM_SCRIM = 2;

    private @ScrimState int mScrimState;
    private int mScrimVerticalMargin;

    private View mScrimView;

    /**
     * Constructor for inflating from XML.
     */
    public ModalDialogLayout(@NonNull Context context) {
        this(context, FULL_SCRIM);
    }

    /**
     * Constructor for initializing views.
     */
    public ModalDialogLayout(@NonNull Context context, @ScrimState int scrimState) {
        super(context);
        mScrimState = scrimState;
        mScrimVerticalMargin =
                getResources().getDimensionPixelSize(R.dimen.tab_modal_scrim_vertical_margin);
        initViews();
    }

    /**
     * Initialize the scrim view and the dialog view.
     */
    private void initViews() {
        mScrimView = new View(getContext());
        mScrimView.setBackgroundResource(R.color.modal_dialog_scrim_color);
        addView(mScrimView);

        MarginLayoutParams params = (MarginLayoutParams) mScrimView.getLayoutParams();
        params.width = MarginLayoutParams.MATCH_PARENT;
        params.height = MarginLayoutParams.MATCH_PARENT;
        params.topMargin = mScrimState == BOTTOM_SCRIM ? mScrimVerticalMargin : 0;
        params.bottomMargin = mScrimState == TOP_SCRIM ? mScrimVerticalMargin : 0;
        mScrimView.setLayoutParams(params);

        inflate(getContext(), R.layout.modal_dialog_view, this);
    }
}
