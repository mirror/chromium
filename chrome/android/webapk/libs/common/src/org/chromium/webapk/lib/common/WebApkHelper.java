// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.lib.common;

import android.content.Context;
import android.content.pm.PackageManager;

/**
 * Provides helper functions that can be shared between Chrome and WebAPKs.
 */
public class WebApkHelper {
    /** Returns the application context of the given package. */
    public static Context getRemoteContext(Context context, String packageName) {
        try {
            return context.getApplicationContext().createPackageContext(
                    packageName, Context.CONTEXT_IGNORE_SECURITY | Context.CONTEXT_INCLUDE_CODE);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        return null;
    }
}
