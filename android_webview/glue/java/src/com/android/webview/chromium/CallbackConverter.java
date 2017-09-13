// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.webkit.ValueCallback;

import org.chromium.base.Callback;

class CallbackConverter {
    public static <T> Callback<T> fromValueCallback(ValueCallback<T> valueCallback) {
        return new Callback<T>() {
            @Override
            public void onResult(T result) {
                valueCallback.onReceiveValue(result);
            }
        };
    }

    public static <T> ValueCallback<T> fromBaseCallback(Callback<T> baseCallback) {
        return new ValueCallback<T>() {
            @Override
            public void onReceiveValue(T result) {
                baseCallback.onResult(result);
            }
        };
    }

    // Do not instantiate this class
    private CallbackConverter() {}
}
