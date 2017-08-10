// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.test;

import android.app.ApplicationPackageManager;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.res.Resources;
import android.os.Bundle;

import org.junit.Assert;
import org.mockito.Mockito;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowApplicationPackageManager;
import org.robolectric.shadows.ShadowPackageManager;

import java.util.HashMap;

/**
 * Helper class for WebAPK JUnit tests.
 */
public class WebApkTestHelper {
    /**
     * CustomShadowApplicationPackageManager allows setting Resources for installed packages.
     *
     * Tests that use this helper class should set
     * @Config(shadows = {WebApkTestHelper$CustomShadowApplicationPackageManager.class})
     * */
    @Implements(value = ApplicationPackageManager.class, inheritImplementationMethods = true)
    public static class CustomShadowApplicationPackageManager
            extends ShadowApplicationPackageManager {
        private final HashMap<String, Resources> mResourceMap;

        public CustomShadowApplicationPackageManager() {
            mResourceMap = new HashMap<>();
        }

        @Override
        @Implementation
        public Resources getResourcesForApplication(String appPackageName)
                throws ApplicationPackageManager.NameNotFoundException {
            Resources result = mResourceMap.get(appPackageName);
            if (result == null) {
                throw new ApplicationPackageManager.NameNotFoundException(appPackageName);
            }
            return result;
        }

        public void setResourcesForTest(String packageName, Resources resources) {
            mResourceMap.put(packageName, resources);
        }
    }

    /**
     * Registers WebAPK. This function also creates an empty resource for the WebAPK.
     * @param packageName The package to register
     * @param metaData Bundle with meta data from WebAPK's Android Manifest.
     */
    public static void registerWebApkWithMetaData(String packageName, Bundle metaData) {
        ShadowApplicationPackageManager packageManager =
                (CustomShadowApplicationPackageManager) Shadows.shadowOf(
                        RuntimeEnvironment.application.getPackageManager());
        assertUsingCustomShadow(packageManager);
        Resources res = Mockito.mock(Resources.class);
        ((CustomShadowApplicationPackageManager) packageManager)
                .setResourcesForTest(packageName, res);
        packageManager.addPackage(newPackageInfo(packageName, metaData));
    }

    /** Sets the resource for the given package name. */
    public static void setResource(String packageName, Resources res) {
        ShadowPackageManager packageManager =
                Shadows.shadowOf(RuntimeEnvironment.application.getPackageManager());
        assertUsingCustomShadow(packageManager);
        ((CustomShadowApplicationPackageManager) packageManager)
                .setResourcesForTest(packageName, res);
    }

    private static PackageInfo newPackageInfo(String packageName, Bundle metaData) {
        ApplicationInfo applicationInfo = new ApplicationInfo();
        applicationInfo.metaData = metaData;
        PackageInfo packageInfo = new PackageInfo();
        packageInfo.packageName = packageName;
        packageInfo.applicationInfo = applicationInfo;
        return packageInfo;
    }

    private static void assertUsingCustomShadow(ShadowPackageManager packageManager) {
        Assert.assertTrue("Tests that use WebApkTestHelper must use the "
                        + "WebApkTestHelper$CustomShadowApplicationPackageManager.",
                packageManager instanceof CustomShadowApplicationPackageManager);
    }
}
