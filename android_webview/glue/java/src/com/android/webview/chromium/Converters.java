// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import org.chromium.base.Callback;

/** A utility that converts between AwContents types and android.webkit types. */
final class Converters {
    private Converters() {}

    public static <T> Callback<T> wrapAsChromiumCallback(
            final android.webkit.ValueCallback<T> callback) {
        return callback == null ? null : value -> callback.onReceiveValue(value);
    }
}
