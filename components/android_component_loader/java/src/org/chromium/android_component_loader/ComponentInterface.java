// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_component_loader;

// import org.chromium.base.BuildInfo;

/** @since 1.0 */
public class ComponentInterface {
    public static String packageLabel() {
        // return BuildInfo.getPackageLabel();
        return "good";
    }
    public static String wow() {
        return "wow";
    }
}