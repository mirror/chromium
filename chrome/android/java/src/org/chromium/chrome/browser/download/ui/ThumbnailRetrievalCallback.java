// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

import android.graphics.Bitmap;

/** The class to call back to after thumbnail is retrieved */
public interface ThumbnailRetrievalCallback {
    void onThumbnailRetrieved(String contentId, Bitmap bitmap);
}