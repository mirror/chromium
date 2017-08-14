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

import java.io.File;
import java.io.IOException;

/**
 * AwVariationsWebViewSeedHandler is used to received the seed data from the
 * AwVariationsConfigurationService on the WebView side. It will copy both the seed data and
 * preference from the service directory into the app directory.
 */
public class AwVariationsWebViewSeedHandler extends Handler {
    private static final String TAG = "AwVariatnsWVHdlr";

    private ServiceConnection mConnection;

    AwVariationsWebViewSeedHandler(ServiceConnection connection) {
        mConnection = connection;
    }

    @Override
    public void handleMessage(Message msg) {
        ContextUtils.getApplicationContext().unbindService(mConnection);
        if (msg.what == AwVariationsConfigurationService.MSG_WITH_SEED_DATA) {
            Bundle data = msg.getData();
            ParcelFileDescriptor seedDataFileDescriptor =
                    data.getParcelable(AwVariationsConfigurationService.KEY_SEED_DATA);
            ParcelFileDescriptor seedPrefFileDescriptor =
                    data.getParcelable(AwVariationsConfigurationService.KEY_SEED_PREF);

            try {
                File variationsDir = getOrCreateVariationsDirectory();
                AwVariationsConfigurationService.copyFileToVariationsDirectory(
                        seedDataFileDescriptor, variationsDir,
                        AwVariationsConfigurationService.SEED_DATA_FILENAME);
                AwVariationsConfigurationService.copyFileToVariationsDirectory(
                        seedPrefFileDescriptor, variationsDir,
                        AwVariationsConfigurationService.SEED_PREF_FILENAME);
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

        if (!AwVariationsConfigurationService.isSeedExpired(appWebViewVariationsDir)) {
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
                msg.what = AwVariationsConfigurationService.MSG_SEED_REQUEST;
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

    // TODO(kmilka): Implement the JNI read seed data and preference function for native code to
    // call.
    @CalledByNative
    public static String getVariationsSeedData() {
        return "";
    }

    @CalledByNative
    public static String getVariationsSeedSignature() {
        return "";
    }

    @CalledByNative
    public static String getVariationsSeedCountry() {
        return "";
    }

    @CalledByNative
    public static String getVariationsSeedDate() {
        return "";
    }

    @CalledByNative
    public static String getVariationsSeedIsGzipCompressed() {
        return "";
    }

    /**
     * Get or create the variations directory in the service directory.
     * @return The variations directory path.
     * @throws IOException if fail to create the directory and get the directory.
     */
    @VisibleForTesting
    public static File getOrCreateVariationsDirectory() throws IOException {
        File appWebViewDir = new File(PathUtils.getDataDirectory());
        File dir = new File(appWebViewDir, AwVariationsConfigurationService.WEBVIEW_VARIATIONS_DIR);
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
