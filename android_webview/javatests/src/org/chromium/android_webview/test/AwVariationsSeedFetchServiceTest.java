// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.annotation.TargetApi;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwVariationsSeedFetchService;
import org.chromium.android_webview.AwVariationsSeedFetchService.SeedPreference;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.IOException;
import java.util.Arrays;

/**
 * Instrumentation tests for AwVariationsSeedFetchService.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@MinAndroidSdkLevel(Build.VERSION_CODES.LOLLIPOP) // Android versions under L don't have JobService.
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsSeedFetchServiceTest {
    @Before
    public void setUp() throws Exception {
        ContextUtils.initApplicationContextForTests(InstrumentationRegistry.getInstrumentation()
                                                            .getTargetContext()
                                                            .getApplicationContext());
    }

    @After
    public void tearDown() throws Exception {
        AwVariationsSeedFetchService.clearDataForTesting();
    }

    /**
     * Ensure reading and writing seed data and pref works correctly.
     */
    @Test
    @MediumTest
    public void testReadAndWriteVariationsSeed() throws IOException, ClassNotFoundException {
        SeedInfo seedInfo = new SeedInfo();
        seedInfo.seedData = "SeedData".getBytes();
        seedInfo.signature = "SeedSignature";
        seedInfo.country = "SeedCountry";
        seedInfo.date = "SeedDate";
        seedInfo.isGzipCompressed = true;
        AwVariationsSeedFetchService.storeVariationsSeed(seedInfo);

        byte[] seedData = AwVariationsSeedFetchService.getVariationsSeedData();
        Assert.assertTrue(Arrays.equals(seedData, seedInfo.seedData));

        SeedPreference seedPreference = AwVariationsSeedFetchService.getVariationsSeedPreference();
        Assert.assertEquals(seedPreference.signature, seedInfo.signature);
        Assert.assertEquals(seedPreference.country, seedInfo.country);
        Assert.assertEquals(seedPreference.date, seedInfo.date);
        Assert.assertEquals(seedPreference.isGzipCompressed, seedInfo.isGzipCompressed);
    }
}
