// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

import org.chromium.base.Callback;
import org.chromium.content.browser.ChildProcessLauncherHelper;

import java.util.List;
import java.util.Map;

/**
 *
 */
public class ChildProcessUtils {
    private ChildProcessUtils() {}

    /**
     * Groups all currently tracked processes by type and returns a map of type -> list of pids.
     *
     * @param callback The callback to notify with the process information.  {@code callback} will
     *                 run on the same thread this method is called on.  That thread must support a
     *                 {@link android.os.Looper}.
     */
    public static void getProcessIdsByType(Callback < Map < String, List<Integer>>> callback) {
        ChildProcessLauncherHelper.getProcessIdsByType(callback);
    }
}