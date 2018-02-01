// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

/**
 * Contains all of the command line switches that are specific to Chromecast on
 * on Android.
 */
public abstract class CastSwitches {
    // Background color to use when chromium hasn't rendered anything yet. This will oftent be
    // displayed briefly when loading a Cast app. Format is a ARGB in hex. (Black: FF000000, Green:
    // FF009000, and so on)
    public static final String CAST_APP_BACKGROUND_COLOR = "cast-app-background-color";

    // Prevent instantiation
    private CastSwitches() {}
}
