// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import java.util.HashMap;

/**
 * Caches WebappInfo objects between their initial creation in either WebappLauncherActivity or in
 * the first run code and their final use in WebappActivity. The caching is done for the sake of:
 * - Correctness. See the comment in WebappActivity#preInflationStartup() for more details.
 * - Speed. WebApkInfo#create() is slow.
 */
public class WebappPendingLaunches {
    /** Initialization-on-demand holder. This exists for thread-safe lazy initialization. */
    private static class Holder {
        private static final HashMap<String, WebappInfo> sMap = new HashMap<String, WebappInfo>();
    }

    public static void addWebappInfo(WebappInfo info) {
        Holder.sMap.put(info.id(), info);
    }

    public static WebappInfo getWebappInfo(String id) {
        return Holder.sMap.get(id);
    }

    public static WebappInfo removeWebappInfo(String id) {
        return Holder.sMap.remove(id);
    }
}
