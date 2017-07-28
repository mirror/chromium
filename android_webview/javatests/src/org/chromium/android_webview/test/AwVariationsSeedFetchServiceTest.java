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

import org.chromium.android_webview.AwVariationsSeedFetchService;
import org.chromium.android_webview.AwVariationsSeedFetchService.SeedPreference;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

/**
 * Instrumentation tests for AwVariationsSeedFetchService on Android L and above.
 */
@MinAndroidSdkLevel(Build.VERSION_CODES.LOLLIPOP)
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsSeedFetchServiceTest extends InstrumentationTestCase {
    protected void setUp() throws Exception {
        ContextUtils.initApplicationContextForTests(
                getInstrumentation().getTargetContext().getApplicationContext());
    }

    protected void tearDown() throws Exception {
        AwVariationsSeedFetchService.clearDataForTesting();
    }

    /**
     * Ensure reading and writing seed data and pref works correctly.
     */
    @MediumTest
    public void testReadAndWriteVariationsSeed() throws IOException, ClassNotFoundException {
        SeedInfo seedInfo = new SeedInfo();
        seedInfo.seedData = "SeedData".getBytes();
        seedInfo.signature = "SeedSignature";
        seedInfo.country = "SeedCountry";
        seedInfo.date = "SeedDate";
        seedInfo.isGzipCompressed = true;
        AwVariationsSeedFetchService.storeVariationsSeed(seedInfo);

        ParcelFileDescriptor fd =
                AwVariationsSeedFetchService.getVariationsSeedDataFileDescriptor();
        String encodedSeedData =
                new BufferedReader(new FileReader(fd.getFileDescriptor())).readLine();
        assertEquals(Base64.encodeToString(seedInfo.seedData, Base64.NO_WRAP), encodedSeedData);

        SeedPreference seedPreference = AwVariationsSeedFetchService.getVariationsSeedPreference();
        assertEquals(seedInfo.signature, seedPreference.signature);
        assertEquals(seedInfo.country, seedPreference.country);
        assertEquals(seedInfo.date, seedPreference.date);
        assertEquals(seedInfo.isGzipCompressed, seedPreference.isGzipCompressed);
    }
}
