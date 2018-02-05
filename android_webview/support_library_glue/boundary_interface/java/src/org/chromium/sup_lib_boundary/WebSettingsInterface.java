// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.sup_lib_boundary;

// Technically this interface is not needed until we add a method to WebSettings with an
// android.webkit parameter or return value. But for forwards compatibility all app-facing classes
// should have a boundary-interface that the WebView glue layer can build against.
/**
 */
public interface WebSettingsInterface {
    void setSafeBrowsingEnabled(boolean enabled);
    boolean getSafeBrowsingEnabled();
}
