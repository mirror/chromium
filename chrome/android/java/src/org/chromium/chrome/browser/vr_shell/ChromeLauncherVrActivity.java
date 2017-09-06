// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.firstrun.FirstRunFlowSequencer;

/**
 * The main purpose of this activity is to handle trusted Daydream intents. It's currently only used
 * to start WebVR auto-presentation but can be extended to handle other VR intents in the future.
 */
public class ChromeLauncherVrActivity extends Activity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (VrIntentUtils.getHandlerInstance().isTrustedDaydreamIntent(getIntent())) {
            // Check if we should push the user through First Run. We set the class of the intent to
            // be the ChromeLauncherActivity because we don't want to start this activity after the
            // first run flow. VR mode can be manually triggered by the user after the FRE.
            Intent newIntent = new Intent(getIntent());
            newIntent.setClassName(this, ChromeLauncherActivity.class.getName());
            if (FirstRunFlowSequencer.launch(this, newIntent, false /* requiresBroadcast */,
                        false /* preferLightweightFre */)) {
                finish();
                return;
            }

            // Set VR specific window mode.
            Intent intent =
                    ChromeLauncherActivity.createCustomTabActivityIntent(this, getIntent(), false);
            intent.setClassName(this, SeparateTaskCustomTabVrActivity.class.getName());
            startActivity(intent, VrIntentUtils.getVrIntentOptions(this));
        }
        finish();
    }
}
