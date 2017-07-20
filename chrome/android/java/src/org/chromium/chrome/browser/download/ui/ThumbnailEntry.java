// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.ui;

/** Contains information related to the thumbnail cached in @{link ThumbnailProviderDiskStorage} */
public class ThumbnailEntry {
    public String contentId;
    public int size;
    public String thumbnailFilePath;

    public ThumbnailEntry(String contentId, int thumbnailSize, String thumbnailFilePath) {
        this.contentId = contentId;
        this.size = thumbnailSize;
        this.thumbnailFilePath = thumbnailFilePath;
    }
}