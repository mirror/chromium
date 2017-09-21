// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.document;

import android.app.Activity;
import android.os.Bundle;
import android.os.StrictMode;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.TraceEvent;
import org.chromium.chrome.browser.ActivityDispatcher;

/**
 * Dispatches incoming intents to the appropriate activity based on the current configuration and
 * Intent fired.
 */
public class ChromeLauncherActivity extends Activity {
    /**
     * Figure out how to route the Intent.  Because this is on the critical path to startup, please
     * avoid making the pathway any more complicated than it already is.  Make sure that anything
     * you add _absolutely has_ to be here.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        // Third-party code adds disk access to Activity.onCreate. http://crbug.com/619824
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        TraceEvent.begin("ChromeLauncherActivity.onCreate");
        try {
            // super.onCreate() is called via the finisher, this is only to avoid the warning
            // about not calling super.onCreate().
            if (false) super.onCreate(savedInstanceState);

            ActivityDispatcher.OnCreateFinisher finisher = createActivityDispatcherFinisher();
            boolean dispatched =
                    new ActivityDispatcher(this, finisher).maybeDispatch(savedInstanceState);
            assert dispatched : "Intent was not dispatched";
            finisher.finishOnCreate(savedInstanceState);
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
            TraceEvent.end("ChromeLauncherActivity.onCreate");
        }
    }

    private ActivityDispatcher.OnCreateFinisher createActivityDispatcherFinisher() {
        return new ActivityDispatcher.OnCreateFinisher() {
            @Override
            public void finishOnCreate(Bundle savedInstanceState) {
                ChromeLauncherActivity.super.onCreate(savedInstanceState);
                ChromeLauncherActivity.this.finish();
            }

            @Override
            public void finishOnCreateRemoveTask(Bundle savedInstanceState) {
                ChromeLauncherActivity.super.onCreate(savedInstanceState);
                ApiCompatibilityUtils.finishAndRemoveTask(ChromeLauncherActivity.this);
            }
        };
    }
}
