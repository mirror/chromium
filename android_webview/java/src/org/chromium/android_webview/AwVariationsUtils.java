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

import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.android_webview.variations.AwVariationsSeedServer;
import org.chromium.android_webview.variations.IAwVariationsSeedServer;
import org.chromium.base.ContextUtils;
import org.chromium.base.CollectionUtil;
import org.chromium.base.FileUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

import java.nio.channels.ReadableByteChannel;

// always storing compressed would require lots of refactoring; compressed/uncompressed seed from
// server is crypto signed

/*
signature (base64)
country
date
is compressed
seed size
seed (binary)
*/

/**
 * Constants and shared methods for fetching and storing the WebView variations seed.
 */
public class AwVariationsUtils {
    private static final String TAG = "AwVariationsUtils";

    //public static final String NEW_SEED_FILE_KEY = "NEW_SEED_FILE";

    private static final int SEED_FILE_VERSION = 1;

    // The expiration time for an app's copy of the Finch seed, after which we'll still use it, but
    // we'll request a new one from the Service.
    private static final long SEED_EXPIRATION_MILLIS = TimeUnit.HOURS.toMillis(6);

    private static Date parseSeedDate(String date) throws ParseException {
        // The SeedInfo.date field comes from the HTTP "Date" header, which has this format.
        // (SimpleDateFormat is weirdly not thread-safe, so instantiate a new one each time.)
        return (new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z", Locale.US)).parse(date);
    }

    public static boolean isExpired(SeedInfo seed) {
        try {
            long now = (new Date()).getTime();
            long expiration = parseSeedDate(seed.date).getTime() + SEED_EXPIRATION_MILLIS;
            return now > expiration;
        } catch (ParseException e) {
            return true;
        }
    }

    /*
    private static File getOrCreateWebViewDir() throws IOException {
        File dir = ContextUtils.getApplicationContext().getFilesDir();
        //File dir = PathUtils.getDataDirectory();
        if (!dir.isDirectory()) {
            if (!dir.mkdir()) {
                throw new IOException("Failed to create WebView files directory");
            }
        }
        return dir;
    }
    */

    public static File getSeedFile() {
        return new File(PathUtils.getDataDirectory(), "variations_seed");
    }

    public static File getNewSeedFile() {
        return new File(PathUtils.getDataDirectory(), "variations_seed_new");
    }

    public static void applyNewSeedFile() {
        File oldSeedFile = getSeedFile();
        File newSeedFile = getNewSeedFile();
        if (!newSeedFile.renameTo(oldSeedFile)) {
            Log.e("blarg", "Failed to replace old seed " + oldSeedFile +
                    " with new seed " + newSeedFile);
        }
    }

    // TODO document format

    // returns null in case of incomplete/corrupt/missing seed
    @Nullable
    public static SeedInfo readSeedFile(File inputFile) {
        SeedInfo info = new SeedInfo();
        try {
            DataInputStream inputData = new DataInputStream(new FileInputStream(inputFile));
            try {
                // Field 1: version
                int version = inputData.readInt();
                if (version != SEED_FILE_VERSION) {
                    Log.e("blarg", "ignoring seed " + inputFile + " - seed version " + version +
                            " is not required version " + SEED_FILE_VERSION);
                    return null;
                }
                Log.e("blarg", "version=" + version);

                // Field 2: SeedInfo.signature
                info.signature = inputData.readUTF();
                Log.e("blarg", "signature=" + info.signature);

                // Field 3: SeedInfo.country
                info.country = inputData.readUTF();
                Log.e("blarg", "country=" + info.country);

                // Field 4: SeedInfo.date (the time this seed was fetched from the network)
                info.date = inputData.readUTF();
                try {
                    parseSeedDate(info.date);
                } catch (ParseException e) {
                    Log.e("blarg", "ignoring seed " + inputFile + " - invalid date " +
                            e.getMessage());
                    return null;
                }
                Log.e("blarg", "date=" + info.date);

                // Field 5: SeedInfo.isGzipCompressed
                info.isGzipCompressed = inputData.readBoolean();
                Log.e("blarg", "isGzipCompressed=" + info.isGzipCompressed);

                // Field 6: seedDataLength
                int seedDataLength = inputData.readInt();
                if (seedDataLength < 1) {
                    Log.e("blarg", "ignoring seed " + inputFile + " - seed data length (" +
                            seedDataLength + ") must be positive");
                    return null;
                }
                // An arbitrary 100 MB maximum should be way bigger than any actual seed.
                if (seedDataLength > 100 * 1024 * 1024) {
                    Log.e("blarg", "ignoring seed " + inputFile + " - seed data length (" +
                            seedDataLength + ") is unreasonably large");
                    return null;
                }
                Log.e("blarg", "seedDataLength=" + seedDataLength);

                // Field 7: SeedInfo.seedData
                info.seedData = new byte[seedDataLength];
                inputData.readFully(info.seedData, 0, seedDataLength);

                // This should be the end of the file. Try to consume 1 more byte, and verify that 0
                // bytes are actually consumed.
                if (inputData.skipBytes(1) != 0) {
                    Log.e("blarg", "ignoring seed " + inputFile + " - junk at end of seed file");
                    return null;
                }
            } catch (EOFException e) {
                Log.e("blarg", "ignoring seed " + inputFile + " - premature end of seed file");
                return null;
            } finally {
                closeStream(inputData);
            }
            return info;
        } catch (IOException e) {
            Log.e("blarg", "failed reading seed: " + e.getMessage());
            return null;
        }
    }

    public static void writeSeedToStream(FileOutputStream out, SeedInfo info) {
        DataOutputStream outData = new DataOutputStream(out);
        try {
            // These must match the order and types of fields written in readSeefFile.
            outData.writeInt(SEED_FILE_VERSION);
            outData.writeUTF(info.signature);
            outData.writeUTF(info.country);
            outData.writeUTF(info.date);
            outData.writeBoolean(info.isGzipCompressed);
            outData.writeInt(info.seedData.length);
            Log.e("blarg", "writeSeedToStream length=" + info.seedData.length +
                    " first=" + info.seedData[0] + " last=" + info.seedData[info.seedData.length-1]);
            outData.write(info.seedData, 0, info.seedData.length);
        } catch (IOException e) {
            Log.e("blarg", "failed writing seed: " + e.getMessage());
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
        public void onBindingDied(ComponentName name) {
            Log.e("blarg", "requestSeed onBindingDied");
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.e("blarg", "requestSeed onServiceConnected");
            try {
                IAwVariationsSeedServer.Stub.asInterface(service).getSeed(mNewSeedFd);
            } catch (RemoteException e) {
                Log.e("blarg", "requestSeed onServiceConnected", e);
            } finally {
                ContextUtils.getApplicationContext().unbindService(this);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.e("blarg", "requestSeed onServiceDisconnected");
        }
    };

    // TODO crashes from service?

    // what if provider changes?

    // TODO rate-limit requets by writing time of last request to seed
    public static void requestSeedFromService() {
        File newSeedFile = getNewSeedFile();
        try {
            newSeedFile.createNewFile(); // silently returns false if it already exists
        } catch (IOException e) {
            Log.e("blarg", "create new seed file failed");
            return;
        }
        Log.e("blarg", "requestSeedFromService newSeedFile=" + newSeedFile);
        ParcelFileDescriptor newSeedFd = null;
        try {
            newSeedFd = ParcelFileDescriptor.open(
                    newSeedFile, ParcelFileDescriptor.MODE_WRITE_ONLY);
        } catch (FileNotFoundException e) {
            Log.e("blarg", "open new seed file failed");
            return;
        }

        Context app = ContextUtils.getApplicationContext();
        /*
        Intent intent = new Intent();
        intent.setClassName(AwBrowserProcess.getWebViewPackageName(),
                AwVariationsSeedServer.class.getName());
        */
        Context provider;
        try {
            provider = app.createPackageContext(AwBrowserProcess.getWebViewPackageName(), 0);
        } catch (NameNotFoundException e) {
            Log.e("blarg", "webview provider not found " + AwBrowserProcess.getWebViewPackageName());
            return;
        }
        Intent intent = new Intent(provider, AwVariationsSeedServer.class);
        /*
        intent.putExtra(NEW_SEED_FILE_KEY, newSeedFd);
        ContextUtils.getApplicationContext().startService(intent);
        */
        SeedServerConnection connection = new SeedServerConnection(newSeedFd);
        if (!app.bindService(intent, connection, Context.BIND_AUTO_CREATE)) {
            Log.e("blarg", "bind failed");
        }
    }

    /**
     * Safely close a stream.
     * @param stream The stream to be closed.
     */
    public static void closeStream(Closeable stream) {
        if (stream != null) {
            try {
                stream.close();
            } catch (IOException e) {
                Log.e(TAG, "Failed to close stream. " + e);
            }
        }
    }
}
