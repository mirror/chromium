// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import android.annotation.TargetApi;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;

import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.VisibleForTesting;

import java.io.IOException;

/**
 * The class is used to received the seed data from the AwVariationsConfigurationService on the
 * WebView side. It will store the both seed data and preference as files into app directory.
 * Handler cannot be create as a non-static class because it may leak.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsWebViewSeedHandler extends Handler {
    private static final String TAG = "AwVariatnsWVHdlr";

    private static final int BUFFER_SIZE = 4096;

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
                File appWebViewVariationsDir = getOrCreateVariationsDirectory();
                copyFileToVariationsDirectory(seedDataFileDescriptor,
                        AwVariationsConfigurationService.SEED_DATA_FILENAME);
                copyFileToVariationsDirectory(seedPrefFileDescriptor,
                        AwVariationsConfigurationService.SEED_PREF_FILENAME);
            } catch (IOException e) {
                Log.e(TAG, "Failed to copy seed from seed directory to app directory. " + e);
            }
        }
    }

    /**
     * Bind to AwVariationsConfigurationService in the current context for WebView package.
     */
    public static void bindToVariationsService() {
        File appWebViewVariationsDir = null;
        try {
            appWebViewVariationsDir = getOrCreateVariationsDirectory();
        } catch (IOException e) {
            Log.e(TAG, "Failed to bind to the variations service. " + e);
        }

        if (appWebViewVariationsDir == null) {
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

    private static File getOrCreateVariationsDirectory() throws IOException {
        File appWebViewDir = new File(PathUtils.getDataDirectory());
        File dir = new File(appWebViewDir, AwVariationsConfigurationService.WEBVIEW_VARIATIONS_DIR);
        if (dir.mkdir() || dir.isDirectory()) {
            return dir;
        }
        throw new IOException("Failed to get or create the WebView variations directory.");
    }

    /**
     * Copy the file in the parcel file descriptor to the variations directory in the app directory.
     * @param fileDescriptor The file descriptor of an opened file.
     * @param fileName The file name for the destination file in the variations directory.
     */
    @VisibleForTesting
    public static void copyFileToVariationsDirectory(
            ParcelFileDescriptor fileDescriptor, String fileName) throws IOException {
        if (fileDescriptor == null) {
            Log.e(TAG, "Variations seed file descriptor is null.");
            return;
        }
        File variationsDir = getOrCreateVariationsDirectory();
        FileInputStream in = null;
        FileOutputStream out = null;
        try {
            File tempFile = File.createTempFile(fileName, null, variationsDir);
            in = new FileInputStream(fileDescriptor.getFileDescriptor());
            out = new FileOutputStream(tempFile);
            byte[] buffer = new byte[BUFFER_SIZE];
            int readCount = 0;
            while ((readCount = in.read(buffer)) > 0) {
                out.write(buffer, 0, readCount);
            }
            AwVariationsConfigurationService.renameTempFile(
                    tempFile, new File(variationsDir, fileName));
        } finally {
            AwVariationsConfigurationService.closeStream(in);
            AwVariationsConfigurationService.closeStream(out);
            try {
                fileDescriptor.close();
            } catch (IOException e) {
                Log.e(TAG, "Failed to close seed data file descriptor. " + e);
            }
        }
    }
}
