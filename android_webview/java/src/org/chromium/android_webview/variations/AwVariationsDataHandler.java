// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * Constants and shared methods for fetching and storing the WebView variations seed.
 */
@JNINamespace("android_webview")
public class AwVariationsDataHandler {
    private static final String TAG = "AwVartnsDataHndlr";

    public static class VariationsSeedData {
        public String data = "";
        public String signature = "";
        public String country = "";
        public long fetchTime = 0;

        public VariationsSeedData() {}

        public VariationsSeedData(
                String dataIn, String signatureIn, String countryIn, long fetchTimeIn) {
            if (countryIn.length() != 2) {
                Log.d(TAG, "variations seed pref country is incorrect length. " + countryIn);
            }

            data = dataIn;
            signature = signatureIn;
            country = countryIn;
            fetchTime = fetchTimeIn;
        }

        @CalledByNative("VariationsSeedData")
        public String getData() {
            return data;
        }

        @CalledByNative("VariationsSeedData")
        public String getSignature() {
            return signature;
        }

        @CalledByNative("VariationsSeedData")
        public String getCountry() {
            return country;
        }

        @CalledByNative("VariationsSeedData")
        public long getFetchTime() {
            return fetchTime;
        }
    }

    @CalledByNative
    public static VariationsSeedData readVariationsSeedData() {
        File webViewVariationsDir = null;
        try {
            webViewVariationsDir = AwVariationsSeedHandler.getOrCreateVariationsDirectory();
        } catch (IOException e) {
            Log.e(TAG, "Failed to obtain WebView data directory. " + e);
            return new VariationsSeedData();
        }

        File seedDataFile = new File(webViewVariationsDir, AwVariationsUtils.SEED_DATA_FILENAME);
        File seedPrefFile = new File(webViewVariationsDir, AwVariationsUtils.SEED_PREF_FILENAME);

        byte[] seedData = new byte[(int) seedDataFile.length()];
        byte[] seedPref = new byte[(int) seedPrefFile.length()];

        FileInputStream inputStream = null;
        try {
            inputStream = new FileInputStream(seedDataFile);
            inputStream.read(seedData);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Failed to find Webview variations seed data file. " + e);
            return new VariationsSeedData();
        } catch (IOException e) {
            Log.e(TAG, "Failed to read vraiations seed data file. " + e);
            return new VariationsSeedData();
        }

        try {
            inputStream = new FileInputStream(seedPrefFile);
            inputStream.read(seedPref);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Failed to find Webview variations seed pref file. " + e);
            return new VariationsSeedData();
        } catch (IOException e) {
            Log.e(TAG, "Failed to read variations seed pref file. " + e);
            return new VariationsSeedData();
        }
        return AwVariationsUtils.decodeVariationsSeedData(seedData, seedPref);
    }
}
