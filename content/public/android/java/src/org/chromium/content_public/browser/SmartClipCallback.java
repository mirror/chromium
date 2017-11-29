// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import android.graphics.Rect;

/**
 * An interface that allows the embedder to be notified when the results of
 * extractSmartClipData are available.
 */
public interface SmartClipCallback {
    /**
     * API that notifies the results of extractSmartClipData.
     *
     * @param text The text data that is extracted for requested smart clip.
     * @param html The html data that is extracted for requested smart clip.
     * @param clipRect The smartclip coordinates in DIP scale which correspond to the
     *                 extracted data.
     */
    void onSmartClipDataExtracted(String text, String html, Rect clipRect);
}
