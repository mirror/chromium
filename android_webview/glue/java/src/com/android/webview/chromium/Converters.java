// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.content.Intent;

/** A utility that converts between AwContents types and android.webkit types. */
final class Converters {
    private Converters() {}

    public static org.chromium.android_webview.AwSettings.ZoomDensity toAwZoomDensity(
            android.webkit.WebSettings.ZoomDensity value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case CLOSE:
                return org.chromium.android_webview.AwSettings.ZoomDensity.CLOSE;
            case FAR:
                return org.chromium.android_webview.AwSettings.ZoomDensity.FAR;
            case MEDIUM:
                return org.chromium.android_webview.AwSettings.ZoomDensity.MEDIUM;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static android.webkit.WebSettings.ZoomDensity fromAwZoomDensity(
            org.chromium.android_webview.AwSettings.ZoomDensity value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case CLOSE:
                return android.webkit.WebSettings.ZoomDensity.CLOSE;
            case FAR:
                return android.webkit.WebSettings.ZoomDensity.FAR;
            case MEDIUM:
                return android.webkit.WebSettings.ZoomDensity.MEDIUM;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }
}
