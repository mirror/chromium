// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.build;

import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Resources;

/**
 * All Java targets that require android have dependence on this class. Add methods that do not
 * require Android to {@link BuildHooks}.
 *
 * This class provides hooks needed when bytecode rewriting. Static convenience methods are used to
 * minimize the amount of code required to be manually generated when bytecode rewriting.
 *
 * This class contains default implementations for all methods and is used when no other
 * implementation is supplied to an android_apk target (via build_hooks_android_impl_deps).
 */
public abstract class BuildHooksAndroid {
    private static final BuildHooksAndroidImpl sInstance = new BuildHooksAndroidImpl();

    public static Resources getResources(Context context) {
        return sInstance.getResourcesImpl(context);
    }

    protected Resources getResourcesImpl(Context context) {
        return null;
    }

    public static AssetManager getAssets(Context context) {
        return sInstance.getAssetsImpl(context);
    }

    protected AssetManager getAssetsImpl(Context context) {
        return null;
    }

    public static Resources.Theme getTheme(Context context) {
        return sInstance.getThemeImpl(context);
    }

    protected Resources.Theme getThemeImpl(Context context) {
        return null;
    }

    public static void setTheme(Context context, int theme) {
        sInstance.setThemeImpl(context, theme);
    }

    protected void setThemeImpl(Context context, int theme) {}

    public static Context createConfigurationContext(Context context) {
        return sInstance.createConfigurationContextImpl(context);
    }

    protected Context createConfigurationContextImpl(Context context) {
        return null;
    }

    public static boolean isEnabled() {
        return sInstance.isEnabledImpl();
    }

    protected boolean isEnabledImpl() {
        return false;
    }

    public static void init() {
        sInstance.initImpl();
    }

    protected void initImpl() {}
}