// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

/**
 * This interface lets the support library glue layer fetch objects from the old glue-layer, without
 * directly depending on that old glue-layer. SharedWebViewProviderState is implemented by
 * WebViewChromium.
 */
public interface SharedWebViewProviderState {
    AwContents getAwContents();

    AwSettings getAwSettings();
}
