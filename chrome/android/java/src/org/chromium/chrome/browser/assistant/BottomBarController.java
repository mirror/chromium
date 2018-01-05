// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant;

import android.view.View;
import android.view.View.OnLayoutChangeListener;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.FrameLayout;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.ui.interpolators.BakedBezierInterpolator;

/**
 * Controls the bottom bar for Assistant actions.
 */
public class BottomBarController {
    private final ChromeActivity mActivity;
    private final ViewGroup mBottomBar;
    private final FrameLayout mBottomBarContainer;

    public BottomBarController(ChromeActivity activity) {
        mActivity = activity;
        mBottomBar = createBottomBarView();
        mBottomBarContainer = (FrameLayout) mBottomBar.findViewById(R.id.container);
    }

    public FrameLayout getContainerView() {
        return mBottomBarContainer;
    }

    private ViewGroup createBottomBarView() {
        ViewStub stub = ((ViewStub) mActivity.findViewById(R.id.assistant_bottombar_stub));
        ViewGroup container = (ViewGroup) stub.inflate();
        container.setVisibility(View.INVISIBLE);
        return container;
    }

    public void show() {
        show(null);
    }

    public void showWhenLayoutReady() {
        // The animation in show() needs to know the final height of the bottom bar, but it hasn't
        // been computed after attaching an action until the first layout is performed.
        // This helper add a listener that will only trigger the show() after a first layout
        // has been done.
        mBottomBarContainer.addOnLayoutChangeListener(new OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                mBottomBarContainer.removeOnLayoutChangeListener(this);
                show();
            }
        });
    }

    public void show(Runnable callback) {
        mBottomBar.setVisibility(View.VISIBLE);
        int height = mBottomBar.getHeight();
        mBottomBar.setTranslationY(height);
        mBottomBar.animate()
                .translationY(0)
                .setInterpolator(BakedBezierInterpolator.TRANSFORM_CURVE)
                .setDuration(400)
                .withEndAction(() -> {
                    if (callback != null) {
                        callback.run();
                    }
                })
                .start();
    }

    public void hide() {
        hide(null);
    }

    public void hide(Runnable callback) {
        int height = mBottomBar.getHeight();
        mBottomBar.setTranslationY(0);
        mBottomBar.animate()
                .translationY(height)
                .setInterpolator(BakedBezierInterpolator.TRANSFORM_CURVE)
                .setDuration(400)
                .withEndAction(() -> {
                    mBottomBar.setVisibility(View.INVISIBLE);
                    if (callback != null) {
                        callback.run();
                    }
                })
                .start();
    }

    public void clear() {
        mBottomBarContainer.removeAllViews();
    }
}
