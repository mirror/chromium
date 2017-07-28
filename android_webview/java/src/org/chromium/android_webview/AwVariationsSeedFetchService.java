// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.annotation.TargetApi;
import android.app.job.JobParameters;
import android.app.job.JobService;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Message;
import android.os.Messenger;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.util.Base64;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.BufferedWriter;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.locks.ReentrantLock;

/**
 * AwVariationsSeedFetchService is a Job Service to fetch test seed data which is used by Finch
 * to enable AB testing experiments in the native code. The fetched data is stored in the local
 * directory which belongs to the Service process. This is a prototype of the Variations Seed Fetch
 * Service which is one part of the work of adding Finch to Android WebView.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsSeedFetchService extends JobService {
    private static final String TAG = "AwVartnsSeedFetchSvc";

    public static final String WEBVIEW_VARIATIONS_DIR = "WebView_Variations/";
    public static final String SEED_DATA_FILENAME = "variations_seed_data";
    public static final String SEED_PREF_FILENAME = "variations_seed_pref";

    private Messenger mMessenger;

    // Synchronization lock to prevent simultaneous local seed file writing.
    private static final ReentrantLock sLock = new ReentrantLock();

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        mMessenger = intent.getParcelableExtra(
                AwVariationsConfigurationService.SEED_FETCH_COMPLETE_NOTIFIER_INTENT_KEY);
        return START_NOT_STICKY;
    }

    @Override
    public boolean onStartJob(JobParameters params) {
        // Ensure we can use ContextUtils later on.
        ContextUtils.initApplicationContext(this.getApplicationContext());
        new FetchFinchSeedDataTask(params).execute();
        return true;
    }

    @Override
    public boolean onStopJob(JobParameters params) {
        // This method is called by the JobScheduler to stop a job before it has finished.
        // Return true here to reschedule the job.
        return true;
    }

    private class FetchFinchSeedDataTask extends AsyncTask<Void, Void, Void> {
        private JobParameters mJobParams;

        FetchFinchSeedDataTask(JobParameters params) {
            mJobParams = params;
        }

        @Override
        protected Void doInBackground(Void... params) {
            fetchVariationsSeed();
            return null;
        }

        @Override
        protected void onPostExecute(Void success) {
            jobFinished(mJobParams, false /* false -> don't reschedule */);
            if (mMessenger != null) {
                try {
                    mMessenger.send(Message.obtain());
                } catch (RemoteException e) {
                    Log.e(TAG, "Failed to send seed fetch complete notification. " + e);
                }
            }
        }
    }

    private static void fetchVariationsSeed() {
        assert !ThreadUtils.runningOnUiThread();
        // TryLock will drop calls from other threads when there is a thread executing the function.
        if (sLock.tryLock()) {
            try {
                SeedInfo seedInfo = VariationsSeedFetcher.get().downloadContent(
                        VariationsSeedFetcher.VariationsPlatform.ANDROID_WEBVIEW, "");
                storeVariationsSeed(seedInfo);
            } catch (IOException e) {
                // Exceptions are handled and logged in the downloadContent method, so we don't
                // need any exception handling here. The only reason we need a catch-statement here
                // is because those exceptions are re-thrown from downloadContent to skip the
                // normal logic flow within that method.
            } finally {
                sLock.unlock();
            }
        }
    }

    /**
     * SeedPreference is used to serialize/deserialize related fields of seed data when reading or
     * writing them to the internal storage.
     */
    public static class SeedPreference implements Serializable {
        /**
         * Let the program deserialize the data when the fields are changed.
         */
        private static final long serialVersionUID = 0L;

        public final String signature;
        public final String country;
        public final String date;
        public final boolean isGzipCompressed;
        public static final int FIELD_SIZE = 4;

        public SeedPreference(SeedInfo seedInfo) {
            signature = seedInfo.signature;
            country = seedInfo.country;
            date = seedInfo.date;
            isGzipCompressed = seedInfo.isGzipCompressed;
        }

        public ArrayList<String> toArrayList() {
            return new ArrayList<String>(
                    Arrays.asList(signature, country, date, Boolean.toString(isGzipCompressed)));
        }
    }

    private static File getOrCreateWebViewVariationsDir() throws IOException {
        File webViewFileDir = ContextUtils.getApplicationContext().getFilesDir();
        File dir = new File(webViewFileDir, WEBVIEW_VARIATIONS_DIR);
        if (dir.mkdir() || dir.isDirectory()) {
            return dir;
        }
        throw new IOException("Failed to get or create the WebView variations directory.");
    }

    /**
     * Store the variations seed data and its related header fields into two separate files in the
     * directory of Android WebView.
     * @param seedInfo The seed data and its related header fields fetched from the finch server.
     */
    @VisibleForTesting
    public static void storeVariationsSeed(SeedInfo seedInfo) {
        BufferedWriter bwSeedData = null;
        ObjectOutputStream oosSeedPref = null;
        try {
            File webViewVariationsDir = getOrCreateWebViewVariationsDir();
            File seedDataFile = File.createTempFile(SEED_DATA_FILENAME, null, webViewVariationsDir);
            bwSeedData = new BufferedWriter(new FileWriter(seedDataFile));
            bwSeedData.write(Base64.encodeToString(seedInfo.seedData, Base64.NO_WRAP));
            renameTempFile(seedDataFile, new File(webViewVariationsDir, SEED_DATA_FILENAME));
            // Store separately so that reading large seed data (which is expensive) can be
            // prevented when checking the last seed fetch time.
            File seedPrefFile = File.createTempFile(SEED_PREF_FILENAME, null, webViewVariationsDir);
            oosSeedPref = new ObjectOutputStream(new FileOutputStream(seedPrefFile));
            oosSeedPref.writeObject(new SeedPreference(seedInfo));
            renameTempFile(seedPrefFile, new File(webViewVariationsDir, SEED_PREF_FILENAME));
        } catch (IOException e) {
            Log.e(TAG, "Failed to write variations seed data or preference to a file." + e);
        } finally {
            closeStream(bwSeedData);
            closeStream(oosSeedPref);
        }
    }

    /**
     * Get the variations seed data from the file in the internal storage.
     * @return The byte array which contains the seed data.
     * @throws IOException if fail to get or create the WebView Variations directory.
     */
    @VisibleForTesting
    public static ParcelFileDescriptor getVariationsSeedDataFileDescriptor() throws IOException {
        File webViewVariationsDir = getOrCreateWebViewVariationsDir();
        return ParcelFileDescriptor.open(new File(webViewVariationsDir, SEED_DATA_FILENAME),
                ParcelFileDescriptor.MODE_READ_ONLY);
    }

    /**
     * Get the variations seed preference from the file in the internal storage.
     * @return The seed preference which holds related header fields of the seed.
     * @throws IOException if fail to get or create the WebView Variations directory.
     * @throws ClassNotFoundException if fail to load the class of the serialized object.
     */
    @VisibleForTesting
    public static SeedPreference getVariationsSeedPreference()
            throws IOException, ClassNotFoundException {
        ObjectInputStream oisSeedPref = null;
        try {
            File webViewVariationsDir = getOrCreateWebViewVariationsDir();
            oisSeedPref = new ObjectInputStream(
                    new FileInputStream(new File(webViewVariationsDir, SEED_PREF_FILENAME)));
            return (SeedPreference) oisSeedPref.readObject();
        } finally {
            if (oisSeedPref != null) {
                oisSeedPref.close();
            }
        }
    }

    /**
     * Clear the test data.
     * @throws IOException if fail to get or create the WebView Variations directory.
     */
    @SuppressFBWarnings("RV_RETURN_VALUE_IGNORED_BAD_PRACTICE") // ignoring File.delete() return
    @VisibleForTesting
    public static void clearDataForTesting() throws IOException {
        File webViewVariationsDir = getOrCreateWebViewVariationsDir();
        File seedDataFile = new File(webViewVariationsDir, SEED_DATA_FILENAME);
        File seedPrefFile = new File(webViewVariationsDir, SEED_PREF_FILENAME);
        seedDataFile.delete();
        seedPrefFile.delete();
        webViewVariationsDir.delete();
    }

    /**
     * Rename a temp file to a new name.
     * @params tempFile The temp file.
     * @params newFile The new file.
     */
    @SuppressFBWarnings("RV_RETURN_VALUE_IGNORED_BAD_PRACTICE")
    // ignoring File.delete() and File.renameTo() return
    public static void renameTempFile(File tempFile, File newFile) {
        newFile.delete();
        tempFile.renameTo(newFile);
    }

    /**
     * Safely close a stream.
     * @params stream The stream to be closed.
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
