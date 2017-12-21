// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant;

import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.Button;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.devtools.DevToolsAgentHost;
import org.chromium.ui.interpolators.BakedBezierInterpolator;

/**
 * Controls the bottom bar for Assistant actions.
 */
public class WebAssistantBottomBarController implements DevToolsAgentHost.EventListener {
    private final ChromeActivity mActivity;
    private final ViewGroup mBottomBar;
    private DevToolsAgentHost mHost;

    public WebAssistantBottomBarController(ChromeActivity activity) {
        mActivity = activity;
        mBottomBar = createBottomBarView();
    }

    public void onDevToolAgentHostReady(DevToolsAgentHost host) {
        mHost = host;
        host.addEventListener(this);
    }

    private ViewGroup createBottomBarView() {
        ViewStub stub = ((ViewStub) mActivity.findViewById(R.id.assistant_bottombar_stub));
        ViewGroup container = (ViewGroup) stub.inflate();
        container.setVisibility(View.INVISIBLE);

        Button button = (Button) container.findViewById(R.id.button);
        button.setText("Debug");
        button.setOnClickListener((v) -> onDebugClicked());

        return container;
    }

    public void toggle() {
        if (mBottomBar.getVisibility() == View.VISIBLE) {
            hide();
        } else {
            show();
        }
    }

    public void show() {
        mBottomBar.setVisibility(View.VISIBLE);
        int height = mBottomBar.getHeight();
        int innerHeight = mBottomBar.findViewById(R.id.container).getHeight();
        mBottomBar.setTranslationY(height);
        mBottomBar.animate()
                .translationY(0)
                .setInterpolator(BakedBezierInterpolator.TRANSFORM_CURVE)
                .setDuration(400)
                // .withEndAction(() -> mFullscreenManager.setBottomControlsHeight(innerHeight))
                .start();
    }

    public void hide() {
        // mFullscreenManager.setBottomControlsHeight(0);
        int height = mBottomBar.getHeight();
        mBottomBar.setTranslationY(0);
        mBottomBar.animate()
                .translationY(height)
                .setInterpolator(BakedBezierInterpolator.TRANSFORM_CURVE)
                .setDuration(400)
                .withEndAction(() -> mBottomBar.setVisibility(View.INVISIBLE))
                .start();
    }

    public void clear() {
        mBottomBar.removeAllViews();
    }

    private void onDebugClicked() {
        if (mHost == null) return;

        android.util.Log.e("foo", "  -- sending Page.enable");
        mHost.sendCommand("Page.enable", null, (result, error) -> {
            if (error != null) {
                android.util.Log.e("foo", "  -- error in callback!: " + error);
                return;
            }
            android.util.Log.e("foo", "  -- sending Page.navigate");
            String params = "{\"url\": \"http://yahoo.com\"}";
            mHost.sendCommand("Page.navigate", params, (result2, error2) -> {
                if (error2 != null) {
                    android.util.Log.e("foo", "  -- error in callback!: " + error2);
                    return;
                }
                android.util.Log.e("foo", "  -- command result: " + result2);
            });
        });
    }

    @Override
    public void onDevToolsEvent(String method, String params) {
        android.util.Log.e("foo", "-- devtools event " + method + ": " + params);
    }
}
