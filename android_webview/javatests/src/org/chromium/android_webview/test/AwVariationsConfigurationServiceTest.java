// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.os.ParcelFileDescriptor;
import android.support.test.filters.MediumTest;
import android.test.InstrumentationTestCase;

import org.chromium.android_webview.AwVariationsConfigurationService;
import org.chromium.android_webview.AwVariationsSeedFetchService;
import org.chromium.android_webview.AwVariationsSeedFetchService.SeedPreference;
import org.chromium.base.ContextUtils;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * Instrumentation tests for AwVariationsConfigurationService.
 */
public class AwVariationsConfigurationServiceTest extends InstrumentationTestCase {
    private SeedInfo mSeedInfo;
    private File mAppDir;
    private static final String APP_DIRNAME = "App_Dir";

    protected void setUp() throws Exception {
        ContextUtils.initApplicationContextForTests(
                getInstrumentation().getTargetContext().getApplicationContext());

        mSeedInfo = new SeedInfo();
        mSeedInfo.seedData = "SeedData".getBytes();
        mSeedInfo.signature = "SeedSignature";
        mSeedInfo.country = "SeedCountry";
        mSeedInfo.date = "SeedDate";
        mSeedInfo.isGzipCompressed = true;
        AwVariationsSeedFetchService.storeVariationsSeed(mSeedInfo);
        mAppDir = getOrCreateTestAppDir(ContextUtils.getApplicationContext().getFilesDir());
    }

    protected void tearDown() throws Exception {
        AwVariationsSeedFetchService.clearDataForTesting();

        File testAppDir = getOrCreateTestAppDir(ContextUtils.getApplicationContext().getFilesDir());
        AwVariationsConfigurationService.clearDataForTesting(testAppDir);
    }

    private static File getOrCreateTestAppDir(File testAppDir) throws IOException {
        File dir = new File(testAppDir, APP_DIRNAME);
        if (dir.mkdir() || dir.isDirectory()) {
            return dir;
        }
        throw new IOException("Failed to get or create the WebView variations dir in app dir.");
    }

    /**
     * Ensure reading and writing seed data in app directory works correctly.
     */
    @MediumTest
    public void testReadAndWriteVariationsSeedDataInAppDir() throws IOException {
        ParcelFileDescriptor parcelFileDescriptor =
                AwVariationsSeedFetchService.getVariationsSeedDataFileDescriptor();
        AwVariationsConfigurationService.storeSeedDataInAppDir(mAppDir, parcelFileDescriptor);
        byte[] seedData = AwVariationsConfigurationService.getSeedDataInAppDir(mAppDir);
        assertTrue(Arrays.equals(mSeedInfo.seedData, seedData));

        parcelFileDescriptor.close();
    }

    /**
     * Ensure reading and writing seed preference in app directory works correctly.
     */
    @MediumTest
    public void testReadAndWriteVariationsSeedPrefInAppDir() throws IOException {
        ArrayList<String> expectedSeedPrefAsList =
                new ArrayList<String>(new SeedPreference(mSeedInfo).toArrayList());
        AwVariationsConfigurationService.storeSeedPrefInAppDir(mAppDir, expectedSeedPrefAsList);
        ArrayList<String> actualSeedPrefAsList =
                AwVariationsConfigurationService.getSeedPrefInAppDir(mAppDir);
        assertEquals(expectedSeedPrefAsList.size(), actualSeedPrefAsList.size());
        for (int i = 0; i < expectedSeedPrefAsList.size(); ++i) {
            assertEquals(expectedSeedPrefAsList.get(i), actualSeedPrefAsList.get(i));
        }
    }
}
