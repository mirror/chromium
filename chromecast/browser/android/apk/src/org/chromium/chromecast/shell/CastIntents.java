// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

/**
 * A namespace for constants to uniquely describe certain public Intents that can be used to control
 * the life cycle of CastWebContentsActivity.
 */
public class CastIntents {
    public static final String ACTION_STOP_ACTIVITY =
            "com.google.android.apps.castshell.intent.action.STOP_ACTIVITY";
    public static final String ACTION_SCREEN_OFF =
            "com.google.android.apps.castshell.intent.action.ACTION_SCREEN_OFF";

    /**
     * Action type of intent from CastWebContentsComponent to activity to show web contents in a
     * CastWebContentsFragment. (To start CastWebContentsActivity use Context.startActivity())
     */
    public static final String ACTION_SHOW_WEB_CONTENT = "com.google.assistant.SHOW_WEB_CONTENT";

    /**
     * Action type of intent from CastWebContentsComponent to mInternalStopReceiver to detach
     * WebContents and then file an intent to CastWebContentsFragment's host activity to stop
     * fragment.
     */
    public static final String ACTION_STOP_WEB_CONTENT_INTERNAL =
            "com.google.assistant.STOP_WEB_CONTENT_INTERNAL";

    /**
     * Action type of intent to host activity of CastWebContentsFragment to stop fragment.
     */
    public static final String ACTION_STOP_WEB_CONTENT = "com.google.assistant.STOP_WEB_CONTENT";
}
