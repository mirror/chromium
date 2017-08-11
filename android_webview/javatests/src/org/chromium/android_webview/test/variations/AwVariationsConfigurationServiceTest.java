// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test.variations;

import static org.chromium.android_webview.variations.AwVariationsConfigurationService.SEED_DATA_FILENAME;
import static org.chromium.android_webview.variations.AwVariationsConfigurationService.SEED_PREF_FILENAME;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.support.test.filters.MediumTest;
import android.test.InstrumentationTestCase;
import android.util.Base64;

import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.android_webview.variations.AwVariationsConfigurationService;
import org.chromium.android_webview.variations.AwVariationsConfigurationService.SeedPreference;
import org.chromium.android_webview.variations.AwVariationsWebViewSeedHandler;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.File;
import java.io.IOException;

/**
 * Instrumentation tests for AwVariationsConfigurationService on Android L and above.
 * Although AwVariationsConfigurationService is not a JobService, the test case still uses the
 * AwVariationsSeedFetchService which can't work under Android L.
 */
@MinAndroidSdkLevel(Build.VERSION_CODES.LOLLIPOP)
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsConfigurationServiceTest extends InstrumentationTestCase {
    private SeedInfo mSeedInfo;

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
    }

    protected void tearDown() throws Exception {
        AwVariationsConfigurationService.clearDataForTesting();
    }

    /**
     * Ensure reading and writing seed data in app directory works correctly.
     */
    @MediumTest
    public void testReadAndWriteSeedData() throws IOException {
        File variationsDir = AwVariationsConfigurationService.getOrCreateVariationsDirectory();
        AwVariationsConfigurationService.storeSeedDataToFile(mSeedInfo.seedData, variationsDir);
        String b64SeedData = AwVariationsConfigurationService.getSeedDataForTesting(variationsDir);
        assertEquals(Base64.encodeToString(mSeedInfo.seedData, Base64.NO_WRAP), b64SeedData);
    }

    /**
     * Ensure reading and writing seed preference in app directory works correctly.
     */
    @MediumTest
    public void testReadAndWriteSeedPref() throws IOException {
        File variationsDir = AwVariationsConfigurationService.getOrCreateVariationsDirectory();
        SeedPreference expect = new SeedPreference(mSeedInfo);
        AwVariationsConfigurationService.storeSeedPrefToFile(expect, variationsDir);
        SeedPreference actual = AwVariationsConfigurationService.getSeedPreference(variationsDir);
        assertEquals(expect.signature, actual.signature);
        assertEquals(expect.country, actual.country);
        assertEquals(expect.date, actual.date);
        assertEquals(expect.isGzipCompressed, actual.isGzipCompressed);
    }

    /**
     * Ensure copying seed data from the service directory to the app directory works correctly.
     */
    @MediumTest
    public void testCopySeedData() throws IOException {
        File serviceDir = AwVariationsConfigurationService.getOrCreateVariationsDirectory();
        AwVariationsConfigurationService.storeSeedDataToFile(mSeedInfo.seedData, serviceDir);

        ParcelFileDescriptor seedDataFileDescriptor =
                AwVariationsConfigurationService.getSeedFileDescriptor(SEED_DATA_FILENAME);
        File appDir = AwVariationsWebViewSeedHandler.getOrCreateVariationsDirectory();
        AwVariationsConfigurationService.copyFileToVariationsDirectory(
                seedDataFileDescriptor, appDir, SEED_DATA_FILENAME);

        String seedDataInServiceDir =
                AwVariationsConfigurationService.getSeedDataForTesting(serviceDir);
        String seedDataInAppDir = AwVariationsConfigurationService.getSeedDataForTesting(appDir);
        assertEquals(seedDataInServiceDir, seedDataInAppDir);
    }

    /**
     * Ensure copying seed preference from the service directory to the app directory works
     * correctly.
     */
    @MediumTest
    public void testCopySeedPref() throws IOException {
        File serviceDir = AwVariationsConfigurationService.getOrCreateVariationsDirectory();
        SeedPreference seedPref = new SeedPreference(mSeedInfo);
        AwVariationsConfigurationService.storeSeedPrefToFile(seedPref, serviceDir);

        ParcelFileDescriptor seedPrefFileDescriptor =
                AwVariationsConfigurationService.getSeedFileDescriptor(SEED_PREF_FILENAME);
        File appDir = AwVariationsWebViewSeedHandler.getOrCreateVariationsDirectory();
        AwVariationsConfigurationService.copyFileToVariationsDirectory(
                seedPrefFileDescriptor, appDir, SEED_PREF_FILENAME);

        SeedPreference seedPrefInServiceDir =
                AwVariationsConfigurationService.getSeedPreference(serviceDir);
        SeedPreference seedPrefInAppDir =
                AwVariationsConfigurationService.getSeedPreference(appDir);
        assertEquals(seedPrefInServiceDir.signature, seedPrefInAppDir.signature);
        assertEquals(seedPrefInServiceDir.country, seedPrefInAppDir.country);
        assertEquals(seedPrefInServiceDir.date, seedPrefInAppDir.date);
        assertEquals(seedPrefInServiceDir.isGzipCompressed, seedPrefInAppDir.isGzipCompressed);
    }
}
