// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.accessibility;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.view.ViewGroup;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.RenderCoordinates;
import org.chromium.content_public.browser.WebContents;

/**
 * Subclass of WebContentsAccessibility for O
 */
@JNINamespace("content")
@TargetApi(Build.VERSION_CODES.O)
public class OWebContentsAccessibility extends LollipopWebContentsAccessibility {
    OWebContentsAccessibility(Context context, ViewGroup containerView, WebContents webContents,
            RenderCoordinates renderCoordinates, boolean shouldFocusOnPageLoad) {
        super(context, containerView, webContents, renderCoordinates, shouldFocusOnPageLoad);
    }
}
