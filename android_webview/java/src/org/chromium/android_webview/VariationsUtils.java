// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.support.annotation.Nullable;

import org.chromium.android_webview.variations.ISeedServer;
import org.chromium.android_webview.variations.SeedServer;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.Closeable;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * Constants and shared methods for fetching and storing the WebView variations seed.
 */
public class VariationsUtils {
    private static final String TAG = "VariationsUtils";

    // Increment this when making breakage changes to the format of the seed file.
    private static final int SEED_FILE_VERSION = 1;

    // Safely close a stream.
    public static void closeStream(Closeable stream) {
        if (stream != null) {
            try {
                stream.close();
            } catch (IOException e) {
                Log.e(TAG, "Failed to close stream " + e);
            }
        }
    }

    private static Date parseSeedDate(String date) throws ParseException {
        // The SeedInfo.date field comes from the HTTP "Date" header, which has this format.
        // (SimpleDateFormat is weirdly not thread-safe, so instantiate a new one each time.)
        return (new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z", Locale.US)).parse(date);
    }

    // Both the WebView variations service and apps using WebView keep a pair of seed files in their
    // data directory. New seeds are written to the new seed file, and then the old file is replaced
    // with the new file.
    public static File getSeedFile() {
        return new File(PathUtils.getDataDirectory(), "variations_seed");
    }

    public static File getNewSeedFile() {
        return new File(PathUtils.getDataDirectory(), "variations_seed_new");
    }

    public static void replaceOldWithNewSeed() {
        File oldSeedFile = getSeedFile();
        File newSeedFile = getNewSeedFile();
        if (!newSeedFile.renameTo(oldSeedFile)) {
            Log.e(TAG, "Failed to replace old seed " + oldSeedFile +
                    " with new seed " + newSeedFile);
        }
    }

    // Returns null in case of incomplete/corrupt/missing seed. The fields read here should match
    // those written in writeSeedToStream. Incremenet SEED_FILE_VERSION if making breaking changes
    // to this format.
    @Nullable
    public static SeedInfo readSeedFile(File inputFile) {
        SeedInfo info = new SeedInfo();
        try {
            DataInputStream inputData = new DataInputStream(new FileInputStream(inputFile));
            try {
                // Field 1: version
                int version = inputData.readInt();
                if (version != SEED_FILE_VERSION) {
                    Log.w(TAG, "Ignoring seed " + inputFile + " - seed version " + version +
                            " is not required version " + SEED_FILE_VERSION);
                    return null;
                }

                // Field 2: SeedInfo.signature
                info.signature = inputData.readUTF();

                // Field 3: SeedInfo.country
                info.country = inputData.readUTF();

                // Field 4: SeedInfo.date (taken from the HTTPS header)
                info.date = inputData.readUTF();
                try {
                    parseSeedDate(info.date);
                } catch (ParseException e) {
                    Log.w(TAG, "Ignoring seed " + inputFile + " - invalid date: \"" +
                            e.getMessage() + "\"");
                    return null;
                }

                // Field 5: SeedInfo.isGzipCompressed
                info.isGzipCompressed = inputData.readBoolean();

                // Field 6: seedDataLength
                int seedDataLength = inputData.readInt();
                if (seedDataLength < 1) {
                    Log.w(TAG, "Ignoring seed " + inputFile + " - seed data length (" +
                            seedDataLength + ") must be positive");
                    return null;
                }
                // An arbitrary 100 MB maximum should be way bigger than any actual seed.
                if (seedDataLength > 100 * 1024 * 1024) {
                    Log.w(TAG, "Ignoring seed " + inputFile + " - seed data length (" +
                            seedDataLength + ") is unreasonably large");
                    return null;
                }

                // Field 7: SeedInfo.seedData
                info.seedData = new byte[seedDataLength];
                inputData.readFully(info.seedData, 0, seedDataLength);
            } catch (EOFException e) {
                Log.w(TAG, "Ignoring seed " + inputFile + " - premature end of seed file");
                return null;
            } finally {
                closeStream(inputData);
            }
            return info;
        } catch (IOException e) {
            Log.e(TAG, "Failed reading seed " + inputFile + " - " + e.getMessage());
            return null;
        }
    }

    // Returns true on success. The fields read here should match those written in readSeedFile.
    // Incremenet SEED_FILE_VERSION if making breaking changes to this format.
    public static boolean writeSeedToStream(FileOutputStream out, SeedInfo info) {
        DataOutputStream outData = new DataOutputStream(out);
        try {
            outData.writeInt(SEED_FILE_VERSION);
            outData.writeUTF(info.signature);
            outData.writeUTF(info.country);
            outData.writeUTF(info.date);
            outData.writeBoolean(info.isGzipCompressed);
            outData.writeInt(info.seedData.length);
            outData.write(info.seedData, 0, info.seedData.length);
            return true;
        } catch (IOException e) {
            Log.e(TAG, "Failed writing seed: " + e.getMessage());
            return false;
        } finally {
            closeStream(outData);
        }
    }

    // This class delivers to a bound service the file descriptor of the seed file the service
    // should write to.
    private static class SeedServerConnection implements ServiceConnection {
        private ParcelFileDescriptor mNewSeedFd;

        public SeedServerConnection(ParcelFileDescriptor newSeedFd) {
            mNewSeedFd = newSeedFd;
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            try {
                ISeedServer.Stub.asInterface(service).getSeed(mNewSeedFd);
            } catch (RemoteException e) {
                Log.e(TAG, "Faild requesting seed", e);
            } finally {
                ContextUtils.getApplicationContext().unbindService(this);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {}
    };

    public static void requestSeedFromService() {
        File newSeedFile = getNewSeedFile();
        try {
            newSeedFile.createNewFile(); // Silently returns false if already exists.
        } catch (IOException e) {
            Log.e(TAG, "Failed to create seed file " + newSeedFile);
            return;
        }
        ParcelFileDescriptor newSeedFd = null;
        try {
            newSeedFd = ParcelFileDescriptor.open(
                    newSeedFile, ParcelFileDescriptor.MODE_WRITE_ONLY);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Failed to open seed file " + newSeedFile);
            return;
        }

        Context app = ContextUtils.getApplicationContext();
        Context provider;
        try {
            provider = app.createPackageContext(AwBrowserProcess.getWebViewPackageName(), 0);
        } catch (NameNotFoundException e) {
            Log.e(TAG, "WebView provider \"" + AwBrowserProcess.getWebViewPackageName() +
                    "\" not found");
            return;
        }
        Intent intent = new Intent(provider, SeedServer.class);
        SeedServerConnection connection = new SeedServerConnection(newSeedFd);
        if (!app.bindService(intent, connection, Context.BIND_AUTO_CREATE)) {
            Log.e(TAG, "Failed to bind to WebView service");
        }
    }
}
