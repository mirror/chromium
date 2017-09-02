// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

/** A utility that converts between AwContents types and android.webkit types. */
final class Converters {
    private Converters() {}

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
}
