// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.vr;

import org.chromium.chrome.browser.vr_shell.ChromeLauncherVrActivity;

/**
 * This class exists so that ChromeLauncherVrActivity can be exported without exposing the
 * implementation details to the callers. We should be using activity-alias for this, but
 * it seems to be broken for the VR usecase (b/65271215). Until the activity-alias
 * implementation is fixed, we add this activity so that we can simply replace it with the
 * alias without breaking callers.
 */
public class VrMain extends ChromeLauncherVrActivity {
    // Do not add code here. Add it to ChromeLauncherVrActivity.
}
