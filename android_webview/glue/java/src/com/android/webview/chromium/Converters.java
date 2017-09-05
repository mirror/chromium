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

    @org.chromium.android_webview.AwSettings.LayoutAlgorithm
    public static int toAwLayoutAlgorithm(android.webkit.WebSettings.LayoutAlgorithm value) {
        switch (value) {
            case NARROW_COLUMNS:
                return org.chromium.android_webview.AwSettings.NARROW_COLUMNS;
            case NORMAL:
                return org.chromium.android_webview.AwSettings.NORMAL;
            case SINGLE_COLUMN:
                return org.chromium.android_webview.AwSettings.SINGLE_COLUMN;
            case TEXT_AUTOSIZING:
                return org.chromium.android_webview.AwSettings.TEXT_AUTOSIZING;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static android.webkit.WebSettings.LayoutAlgorithm fromAwLayoutAlgorithm(
            @org.chromium.android_webview.AwSettings.LayoutAlgorithm int value) {
        switch (value) {
            case org.chromium.android_webview.AwSettings.NARROW_COLUMNS:
                return android.webkit.WebSettings.LayoutAlgorithm.NARROW_COLUMNS;
            case org.chromium.android_webview.AwSettings.NORMAL:
                return android.webkit.WebSettings.LayoutAlgorithm.NORMAL;
            case org.chromium.android_webview.AwSettings.SINGLE_COLUMN:
                return android.webkit.WebSettings.LayoutAlgorithm.SINGLE_COLUMN;
            case org.chromium.android_webview.AwSettings.TEXT_AUTOSIZING:
                return android.webkit.WebSettings.LayoutAlgorithm.TEXT_AUTOSIZING;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }
}
