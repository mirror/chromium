// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import android.os.ParcelFileDescriptor;

import org.chromium.base.FileUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/**
 * Constants and shared methods for fetching and storing the WebView variations seed.
 */
public class AwVariationsUtils {
    private static final String TAG = "AwVariationsUtils";

    // Types for the WebViews to identify the message. If it is MSG_WITH_SEED_DATA, the WebView will
    // store the seed in the message to the app directory.
    public static final int MSG_WITHOUT_SEED_DATA = 0;
    public static final int MSG_WITH_SEED_DATA = 1;

    // Types for the service to identify the message which Context binds to it.
    public static final int MSG_SEED_REQUEST = 2;
    public static final int MSG_SEED_TRANSFER = 3;

    // The file names used to store the variations seed.
    public static final String SEED_DATA_FILENAME = "variations_seed_data";
    public static final String SEED_PREF_FILENAME = "variations_seed_pref";

    // The key string used in the message to store the variations seed.
    public static final String KEY_SEED_DATA = "KEY_SEED_DATA";
    public static final String KEY_SEED_PREF = "KEY_SEED_PREF";

    // The expiration time for the Finch seed.
    public static final long SEED_EXPIRATION_TIME_IN_MILLIS = TimeUnit.MINUTES.toMillis(30);

    // Buffer size when copying seed from service directory to app directory.
    private static final int BUFFER_SIZE = 4096;

    public static final ReentrantReadWriteLock sSeedDataLock = new ReentrantReadWriteLock();
    public static final ReentrantReadWriteLock sSeedPrefLock = new ReentrantReadWriteLock();

    /**
     * Judge if the current Finch seed in a given directory is expired.
     * @return True if the seed is expired or missing, otherwise false.
     */
    public static boolean isSeedExpired(File variationsDir) {
        Date lastSeedFetchTime = getLastSeedFetchTime(variationsDir);
        if (lastSeedFetchTime == null) {
            return true;
        }
        return new Date().getTime() - lastSeedFetchTime.getTime() > SEED_EXPIRATION_TIME_IN_MILLIS;
    }

    /**
     * Copy the file in the parcel file descriptor to the variations directory in the app directory.
     * @param fileDescriptor The file descriptor of an opened file.
     * @param fileName The name for the destination file in the variations directory.
     * @throws IOException if fail to get the directory or copy file to the aoo
     */
    @VisibleForTesting
    public static void copyFileToVariationsDirectory(ParcelFileDescriptor fileDescriptor,
            File variationsDir, String fileName) throws IOException {
        if (fileDescriptor == null) {
            Log.e(TAG, "Variations seed file descriptor is null.");
            return;
        }
        ReentrantReadWriteLock rwLock = getLockByName(fileName);
        Lock lock = rwLock.readLock();
        FileInputStream in = null;
        FileOutputStream out = null;
        try {
            lock.lock();
            File tempFile = File.createTempFile(fileName, null, variationsDir);
            in = new FileInputStream(fileDescriptor.getFileDescriptor());
            out = new FileOutputStream(tempFile);
            byte[] buffer = new byte[BUFFER_SIZE];
            int readCount = 0;
            while ((readCount = in.read(buffer)) > 0) {
                out.write(buffer, 0, readCount);
            }
            renameTempFile(tempFile, new File(variationsDir, fileName));
        } finally {
            lock.unlock();
            closeStream(in);
            closeStream(out);
            try {
                fileDescriptor.close();
            } catch (IOException e) {
                Log.e(TAG, "Failed to close seed data file descriptor. " + e);
            }
        }
    }

    /**
     * Rename a temp file to a new name.
     * @param tempFile The temp file.
     * @param newFile The new file.
     */
    public static void renameTempFile(File tempFile, File newFile) {
        ThreadUtils.setThreadAssertsDisabledForTesting(true);
        FileUtils.recursivelyDeleteFile(newFile);
        boolean isTempFileRenamed = tempFile.renameTo(newFile);
        if (!isTempFileRenamed) {
            Log.e(TAG,
                    "Failed to rename " + tempFile.getAbsolutePath() + " to "
                            + newFile.getAbsolutePath() + ".");
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

    private static Date getLastSeedFetchTime(File variationsDir) {
        Date lastSeedFetchTime = null;
        try {
            SeedPreference seedPref = getSeedPreference(variationsDir);
            lastSeedFetchTime = parseDateTimeString(seedPref.date);
        } catch (IOException e) {
            Log.e(TAG, "Failed to read variations seed preference. " + e);
        }
        return lastSeedFetchTime;
    }

    private static ReentrantReadWriteLock getLockByName(String fileName) {
        if (AwVariationsUtils.SEED_DATA_FILENAME.equals(fileName)) return sSeedDataLock;
        return sSeedPrefLock;
    }

    /**
     * SeedPreference is used to serialize/deserialize related fields of seed data when reading or
     * writing them into file.
     */
    public static class SeedPreference {
        public static final int FIELD_SIZE = 4;

        public final String signature;
        public final String country;
        public final String date;
        public final boolean isGzipCompressed;

        private SeedPreference(List<String> seedPrefAsList) {
            signature = seedPrefAsList.get(0);
            country = seedPrefAsList.get(1);
            date = seedPrefAsList.get(2);
            isGzipCompressed = Boolean.parseBoolean(seedPrefAsList.get(3));
        }

        public SeedPreference(SeedInfo seedInfo) {
            signature = seedInfo.signature;
            country = seedInfo.country;
            date = seedInfo.date;
            isGzipCompressed = seedInfo.isGzipCompressed;
        }

        public List<String> toList() {
            return Arrays.asList(signature, country, date, Boolean.toString(isGzipCompressed));
        }

        public static SeedPreference fromList(List<String> seedPrefAsList) {
            if (seedPrefAsList.size() != FIELD_SIZE) {
                Log.e(TAG,
                        "Failed to validate the seed preference(the field number is not correct).");
                return null;
            }
            return new SeedPreference(seedPrefAsList);
        }
    }

    /**
     * Get the variations seed preference in a given directory.
     * @return The seed preference.
     * @throws IOException if fail to read the seed preference file in the given directory.
     */
    @VisibleForTesting
    public static SeedPreference getSeedPreference(File variationsDir) throws IOException {
        BufferedReader reader = null;
        File seedPrefFile = new File(variationsDir, AwVariationsUtils.SEED_PREF_FILENAME);
        try {
            sSeedPrefLock.readLock().lock();
            reader = new BufferedReader(new FileReader(seedPrefFile));
            ArrayList<String> seedPrefAsList = new ArrayList<String>();
            String line = null;
            while ((line = reader.readLine()) != null) {
                seedPrefAsList.add(line);
            }
            return AwVariationsUtils.SeedPreference.fromList(seedPrefAsList);
        } finally {
            sSeedPrefLock.readLock().unlock();
            closeStream(reader);
        }
    }

    /**
     * Parse the string containing date time info to a Date object.
     * @param dateTimeStr The string holds the date time info.
     * @return The Date object holds the date info from the input string, null if parse failed.
     */
    private static Date parseDateTimeString(String dateTimeStr) {
        Date lastSeedFetchTime = null;
        // The date format comes from the date field in the HTTP header of the seed.
        SimpleDateFormat format = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z", Locale.US);
        try {
            lastSeedFetchTime = format.parse(dateTimeStr);
        } catch (ParseException e) {
            Log.e(TAG, "Failed to parse the time. " + e);
        }
        return lastSeedFetchTime;
    }
}
