// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.app.Activity;
import android.os.Bundle;

import org.chromium.base.Log;

public class NoDisplayActivity extends Activity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.wtf("TAG", "WINDOW MAYBE");
        overridePendingTransition(0, 0);
        finish();
        overridePendingTransition(0, 0);
    }
}