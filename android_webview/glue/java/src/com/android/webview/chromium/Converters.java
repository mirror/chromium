// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.content.Intent;

/** A utility that converts between AwContents types and android.webkit types. */
final class Converters {
    private Converters() {}

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

    public static org.chromium.android_webview.AwSettings.PluginState toAwPluginState(
            android.webkit.WebSettings.PluginState value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case OFF:
                return org.chromium.android_webview.AwSettings.PluginState.OFF;
            case ON:
                return org.chromium.android_webview.AwSettings.PluginState.ON;
            case ON_DEMAND:
                return org.chromium.android_webview.AwSettings.PluginState.ON_DEMAND;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static android.webkit.WebSettings.PluginState fromAwPluginState(
            org.chromium.android_webview.AwSettings.PluginState value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case OFF:
                return android.webkit.WebSettings.PluginState.OFF;
            case ON:
                return android.webkit.WebSettings.PluginState.ON;
            case ON_DEMAND:
                return android.webkit.WebSettings.PluginState.ON_DEMAND;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static <T> org.chromium.android_webview.AwValueCallback<T> wrapAsAwValueCallback(
            final android.webkit.ValueCallback<T> callback) {
        if (callback == null) {
            return null;
        }
        return value -> callback.onReceiveValue(value);
    }

    // Note: This method is necessary because AwContents sends a callback to android.webkit code to
    // implement getVisitedHistory. To avoid a dependency from AwContents to
    // android.webkit.ValueCallback, we need to wrap that AwValueCallback as a ValueCallback.
    public static <T> android.webkit.ValueCallback<T> wrapAsValueCallback(
            final org.chromium.android_webview.AwValueCallback<T> callback) {
        if (callback == null) {
            return null;
        }
        return value -> callback.onReceiveValue(value);
    }

    public static android.webkit.ConsoleMessage fromAwConsoleMessage(
            org.chromium.android_webview.AwConsoleMessage value) {
        if (value == null) {
            return null;
        }
        return new android.webkit.ConsoleMessage(value.message(), value.sourceId(),
                value.lineNumber(), fromAwMessageLevel(value.messageLevel()));
    }

    private static android.webkit.ConsoleMessage.MessageLevel fromAwMessageLevel(
            org.chromium.android_webview.AwConsoleMessage.MessageLevel value) {
        if (value == null) {
            return null;
        }
        switch (value) {
            case TIP:
                return android.webkit.ConsoleMessage.MessageLevel.TIP;
            case LOG:
                return android.webkit.ConsoleMessage.MessageLevel.LOG;
            case WARNING:
                return android.webkit.ConsoleMessage.MessageLevel.WARNING;
            case ERROR:
                return android.webkit.ConsoleMessage.MessageLevel.ERROR;
            case DEBUG:
                return android.webkit.ConsoleMessage.MessageLevel.DEBUG;
            default:
                throw new IllegalArgumentException("Unsupported value: " + value);
        }
    }

    public static android.webkit.GeolocationPermissions.Callback
            fromAwGeolocationPermissionsCallback(
            final org.chromium.android_webview.AwGeolocationPermissions.Callback callback) {
        if (callback == null) {
            return null;
        }
        return new android.webkit.GeolocationPermissions.Callback() {
            @Override
            public void invoke(String origin, boolean allow, boolean retain) {
                callback.invoke(origin, allow, retain);
            }
        };
    }
}
