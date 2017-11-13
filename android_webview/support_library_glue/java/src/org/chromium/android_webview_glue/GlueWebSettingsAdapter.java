// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview_glue;

import org.chromium.android_webview.AwSettings;
import org.chromium.sup_lib_boundary.WebSettingsInterface;

// TODO we can probably auto-generate this from ContentSettingsAdapter?
class GlueWebSettingsAdapter implements WebSettingsInterface {
    private AwSettings mAwSettings;

    GlueWebSettingsAdapter(AwSettings awSettings) {
        mAwSettings = awSettings;
    }

    @Override
    public boolean getSafeBrowsingEnabled() {
        return mAwSettings.getSafeBrowsingEnabled();
    }

    @Override
    public void setSafeBrowsingEnabled(boolean enabled) {
        mAwSettings.setSafeBrowsingEnabled(enabled);
    }
}
