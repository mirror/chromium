// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

/**
 * A mirror of {@link android.webkit.ValueCallback}.
 *
 * @param <T> the type of the value.
 */
public interface AwValueCallback<T> {
    /**
     * Invoked when the value is available.
     * @param value The value.
     */
    public void onReceiveValue(T value);
}
