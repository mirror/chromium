// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;

import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.base.ContextUtils;
import org.chromium.base.FileUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

/**
 * AwVariationsWebViewSeedHandler is used to received the seed data from the
 * AwVariationsConfigurationService on the WebView side. It will copy both the seed data and
 * preference from the service directory into the app directory.
 */
public class AwVariationsWebViewSeedHandler extends Handler {
    private static final String TAG = "AwVariatnsWVHdlr";

    // Cache variations seed meta-data so a series of jni calls won't all read the data from
    // storage.
    private static Boolean sHasCachedPrefData = false;
    private static String sVariationsSeedDate = "";
    private static String sVariationsSeedSignature = "";
    private static String sVariationsSeedCountry = "";

    private ServiceConnection mConnection;

    AwVariationsWebViewSeedHandler(ServiceConnection connection) {
        mConnection = connection;
    }

    @Override
    public void handleMessage(Message msg) {
        ContextUtils.getApplicationContext().unbindService(mConnection);
        if (msg.what == AwVariationsUtils.MSG_WITH_SEED_DATA) {
            Bundle data = msg.getData();
            ParcelFileDescriptor seedDataFileDescriptor =
                    data.getParcelable(AwVariationsUtils.KEY_SEED_DATA);
            ParcelFileDescriptor seedPrefFileDescriptor =
                    data.getParcelable(AwVariationsUtils.KEY_SEED_PREF);

            try {
                File variationsDir = getOrCreateVariationsDirectory();
                AwVariationsUtils.copyFileToVariationsDirectory(seedDataFileDescriptor,
                        variationsDir, AwVariationsUtils.SEED_DATA_FILENAME);
                AwVariationsUtils.copyFileToVariationsDirectory(seedPrefFileDescriptor,
                        variationsDir, AwVariationsUtils.SEED_PREF_FILENAME);
            } catch (IOException e) {
                Log.e(TAG, "Failed to copy seed from seed directory to app directory. " + e);
            }
        }
    }

    /**
     * Bind to AwVariationsConfigurationService in the current context to ask for a seed.
     */
    public static void bindToVariationsService() {
        File appWebViewVariationsDir = null;
        try {
            appWebViewVariationsDir = getOrCreateVariationsDirectory();
        } catch (IOException e) {
            Log.e(TAG, "Failed to bind to the variations service. " + e);
            return;
        }

        if (!AwVariationsUtils.isSeedExpired(appWebViewVariationsDir)) {
            Log.d(TAG, "The current Finch seed is not expired, so no need to ask for a new seed.");
            return;
        }
        Intent intent = new Intent();
        String webViewPackageName = AwBrowserProcess.getWebViewPackageName();
        intent.setClassName(webViewPackageName, AwVariationsConfigurationService.class.getName());
        ServiceConnection connection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                Message msg = Message.obtain();
                msg.what = AwVariationsUtils.MSG_SEED_REQUEST;
                msg.replyTo = new Messenger(new AwVariationsWebViewSeedHandler(this));
                try {
                    // Send a message to request for a new seed
                    new Messenger(service).send(msg);
                } catch (RemoteException e) {
                    Log.e(TAG, "Failed to seed message to AwVariationsConfigurationService. " + e);
                }
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {}
        };

        Context context = ContextUtils.getApplicationContext();
        if (!context.bindService(intent, connection, Context.BIND_AUTO_CREATE)) {
            Log.w(TAG, "Could not bind to AwVariationsConfigurationService. " + intent);
        }
    }

    @CalledByNative
    public static String getVariationsSeedData() throws IOException {
        File webViewVariationsDir = getOrCreateVariationsDirectory();
        File seedDataFile = new File(webViewVariationsDir, AwVariationsUtils.SEED_DATA_FILENAME);

        BufferedReader seedDataReader = new BufferedReader(new FileReader(seedDataFile));
        String seedData = "";
        try {
            seedData = seedDataReader.readLine();
        } finally {
            seedDataReader.close();
        }
        return seedData;
    }

    private static void readVariationsSeedPref() throws IOException {
        if (sHasCachedPrefData) return;

        File webViewVariationsDir = getOrCreateVariationsDirectory();
        File seedPrefFile = new File(webViewVariationsDir, AwVariationsUtils.SEED_PREF_FILENAME);
        BufferedReader seedPrefReader = new BufferedReader(new FileReader(seedPrefFile));

        try {
            sVariationsSeedSignature = seedPrefReader.readLine();
            sVariationsSeedCountry = seedPrefReader.readLine();
            sVariationsSeedDate = seedPrefReader.readLine();
        } finally {
            seedPrefReader.close();
        }

        sHasCachedPrefData = true;
    }

    @CalledByNative
    public static String getVariationsSeedSignature() throws IOException {
        readVariationsSeedPref();
        return sVariationsSeedSignature;
    }

    @CalledByNative
    public static String getVariationsSeedCountry() throws IOException {
        readVariationsSeedPref();
        return sVariationsSeedCountry;
    }

    @CalledByNative
    public static String getVariationsSeedDate() throws IOException {
        readVariationsSeedPref();
        return sVariationsSeedDate;
    }

    /**
     * Get or create the variations directory in the service directory.
     * @return The variations directory path.
     * @throws IOException if fail to create the directory and get the directory.
     */
    @VisibleForTesting
    public static File getOrCreateVariationsDirectory() throws IOException {
        File dir = new File(PathUtils.getDataDirectory());

        if (dir.mkdir() || dir.isDirectory()) {
            return dir;
        }
        throw new IOException("Failed to get or create the WebView variations directory.");
    }

    /**
     * Clear the test data.
     * @throws IOException if fail to get or create the WebView variations directory.
     */
    @VisibleForTesting
    public static void clearDataForTesting() throws IOException {
        File variationsDir = getOrCreateVariationsDirectory();
        FileUtils.recursivelyDeleteFile(variationsDir);
    }
}
