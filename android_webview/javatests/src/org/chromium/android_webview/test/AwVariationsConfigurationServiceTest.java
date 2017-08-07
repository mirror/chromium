// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.support.test.filters.MediumTest;
import android.test.InstrumentationTestCase;
import android.util.Base64;

import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.android_webview.AwVariationsConfigurationService;
import org.chromium.android_webview.AwVariationsSeedFetchService;
import org.chromium.android_webview.AwVariationsSeedFetchService.SeedPreference;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.IOException;
import java.util.ArrayList;

/**
 * Instrumentation tests for AwVariationsConfigurationService on Android L and above.
 * Although AwVariationsConfigurationService is not a JobService, the test case still uses the
 * AwVariationsSeedFetchService which can't work on Android K.
 */
@MinAndroidSdkLevel(Build.VERSION_CODES.LOLLIPOP)
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsConfigurationServiceTest extends InstrumentationTestCase {
    private SeedInfo mSeedInfo;
    private static final String APP_DIRNAME = "App_Dir";

    protected void setUp() throws Exception {
        ContextUtils.initApplicationContextForTests(
                getInstrumentation().getTargetContext().getApplicationContext());
        PathUtils.setPrivateDataDirectorySuffix(AwBrowserProcess.PRIVATE_DATA_DIRECTORY_SUFFIX);

        mSeedInfo = new SeedInfo();
        mSeedInfo.seedData = "SeedData".getBytes();
        mSeedInfo.signature = "SeedSignature";
        mSeedInfo.country = "SeedCountry";
        mSeedInfo.date = "SeedDate";
        mSeedInfo.isGzipCompressed = true;
        AwVariationsSeedFetchService.storeVariationsSeed(mSeedInfo);
    }

    protected void tearDown() throws Exception {
        AwVariationsSeedFetchService.clearDataForTesting();
        AwVariationsConfigurationService.clearDataForTesting();
    }

    /**
     * Ensure reading and writing seed data in app directory works correctly.
     */
    @MediumTest
    public void testReadAndWriteVariationsSeedDataInAppDir() throws IOException {
        ParcelFileDescriptor parcelFileDescriptor =
                AwVariationsSeedFetchService.getVariationsSeedDataFileDescriptor();
        AwVariationsConfigurationService.storeSeedDataInAppDir(parcelFileDescriptor);
        String encodedSeedData = AwVariationsConfigurationService.getSeedDataInAppDir();
        assertEquals(Base64.encodeToString(mSeedInfo.seedData, Base64.NO_WRAP), encodedSeedData);

        parcelFileDescriptor.close();
    }

    /**
     * Ensure reading and writing seed preference in app directory works correctly.
     */
    @MediumTest
    public void testReadAndWriteVariationsSeedPrefInAppDir() throws IOException {
        ArrayList<String> expectedSeedPrefAsList =
                new ArrayList<String>(new SeedPreference(mSeedInfo).toArrayList());
        AwVariationsConfigurationService.storeSeedPrefInAppDir(expectedSeedPrefAsList);
        ArrayList<String> actualSeedPrefAsList =
                AwVariationsConfigurationService.getSeedPrefInAppDir();
        assertEquals(expectedSeedPrefAsList.size(), actualSeedPrefAsList.size());
        for (int i = 0; i < expectedSeedPrefAsList.size(); ++i) {
            assertEquals(expectedSeedPrefAsList.get(i), actualSeedPrefAsList.get(i));
        }
    }
}
