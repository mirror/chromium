// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

/**
 * A delegate that calls back to {@link ThumbnailProviderImpl}.
 */
public interface ThumbnailProviderImplDelegate {
    void retrieveThumbnail(ThumbnailProvider.ThumbnailRequest request,
            ThumbnailProviderGeneratorObserver observer);
    void destroy();
}