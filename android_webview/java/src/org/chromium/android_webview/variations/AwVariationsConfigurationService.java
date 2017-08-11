// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.variations;

import android.annotation.TargetApi;
import android.app.Service;
import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.util.Base64;

import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.base.ContextUtils;
import org.chromium.base.FileUtils;
import org.chromium.base.Log;
import org.chromium.base.PathUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
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
 * AwVariationsConfigurationService is a bound service that shares the Finch seed with all the
 * WebViews on the system. When a WebView binds to the service, it will immediately return the local
 * seed if it is not expired. Otherwise it will schedule a seed fetch job to the job service and
 * return the empty message just to make the WebViews unbind from the service. When multiple
 * WebViews ask for the seed, the service will schedule the first one. The mechanism is ensured by
 * checking if there is a current seed fetch job running, if there is pending job and if the seed is
 * expired.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsConfigurationService extends Service {
    private static final String TAG = "AwVariatnsConfigSvc";

    // Types for the WebViews to identify the message. If it is MSG_WITH_SEED_DATA, the WebView will
    // store the seed in the message to the app directory.
    public static final int MSG_WITHOUT_SEED_DATA = 0;
    public static final int MSG_WITH_SEED_DATA = 1;

    // Types for the service to identify the message which Context binds to it.
    public static final int MSG_SEED_REQUEST = 0;
    public static final int MSG_SEED_TRANSFER = 1;

    // The file names used to store the variations seed.
    public static final String SEED_DATA_FILENAME = "variations_seed_data";
    public static final String SEED_PREF_FILENAME = "variations_seed_pref";

    // The key string used in the message to store the variations seed.
    public static final String KEY_SEED_DATA = "KEY_SEED_DATA";
    public static final String KEY_SEED_PREF = "KEY_SEED_PREF";

    // The name of the directory where we store the variations seed.
    public static final String WEBVIEW_VARIATIONS_DIR = "WebView_Variations";

    // The expiration time for the Finch seed.
    public static final long SEED_EXPIRATION_TIME_IN_MILLIS = TimeUnit.MINUTES.toMillis(30);

    // Messenger to handle seed requests from the WebView.
    private final Messenger mOnBindMessenger = new Messenger(new OnBindHandler());

    // Buffer size when copying seed from service directory to app directory.
    private static final int BUFFER_SIZE = 4096;

    // Protect the isScheduling variable to be atomic in read and write
    private static final Object sScheduleLock = new Object();
    private static boolean sIsScheduling = false;

    // Protect the seed data and pref file when read and write.
    private static final ReentrantReadWriteLock sSeedDataLock = new ReentrantReadWriteLock();
    private static final ReentrantReadWriteLock sSeedPrefLock = new ReentrantReadWriteLock();

    @Override
    public void onCreate() {
        super.onCreate();
        // Ensure that ContextUtils.getApplicationContext() will return the current context in the
        // following code for the function won't work when we use the standalone WebView package.
        ContextUtils.initApplicationContext(this.getApplicationContext());
        // Ensure that PathUtils.getDataDirectory() will return the app_webview directory in the
        // following code.
        PathUtils.setPrivateDataDirectorySuffix(AwBrowserProcess.PRIVATE_DATA_DIRECTORY_SUFFIX);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mOnBindMessenger.getBinder();
    }

    private static class OnBindHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == MSG_SEED_REQUEST) {
                AwVariationsConfigurationService.handleSeedRequest(msg);
            } else if (msg.what == MSG_SEED_TRANSFER) {
                AwVariationsConfigurationService.handleSeedTransfer(msg);
            }
        }
    };

    // Handle the seed transfer call from the Seed Fetch Service. It will store the fetched seed
    // into separate two files in the service directory.
    private static void handleSeedTransfer(Message msg) {
        Bundle data = msg.getData();
        byte[] seedData = data.getByteArray(KEY_SEED_DATA);
        ArrayList<String> seedPrefAsList = data.getStringArrayList(KEY_SEED_PREF);
        try {
            File webViewVariationsDir = getOrCreateVariationsDirectory();
            storeSeedDataToFile(seedData, webViewVariationsDir);
            storeSeedPrefToFile(SeedPreference.fromList(seedPrefAsList), webViewVariationsDir);
            synchronized (sScheduleLock) {
                sIsScheduling = false;
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed to write seed to the service directory. " + e);
        }
    }

    // Handle the seed request call from the WebView. It will decide whether to schedule a new seed
    // fetch job or return the seed in the service directory.
    private static void handleSeedRequest(Message msg) {
        // TODO: add tests for this logic:
        // 1. There are only one seed fetch job once per time
        // 2. Send back empty message if the seed is expired
        // 2. Send back message with data if the seed is not expired
        try {
            synchronized (sScheduleLock) {
                File webViewVariationsDir = getOrCreateVariationsDirectory();
                if (!sIsScheduling && isSeedExpired(webViewVariationsDir) && !existPendingJob()) {
                    scheduleSeedFetchJob();
                    sIsScheduling = true;
                    sendMessageToWebView(msg.replyTo, MSG_WITHOUT_SEED_DATA);
                } else {
                    sendMessageToWebView(msg.replyTo, MSG_WITH_SEED_DATA);
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed to handle seed request. " + e);
            sendMessageToWebView(msg.replyTo, MSG_WITHOUT_SEED_DATA);
        }
    }

    private static boolean existPendingJob() {
        Context context = ContextUtils.getApplicationContext();
        JobScheduler scheduler =
                (JobScheduler) context.getSystemService(Context.JOB_SCHEDULER_SERVICE);
        return scheduler.getPendingJob(TaskIds.WEBVIEW_VARIATIONS_SEED_FETCH_JOB_ID) != null;
    }

    private static void scheduleSeedFetchJob() {
        Context context = ContextUtils.getApplicationContext();
        JobScheduler scheduler =
                (JobScheduler) context.getSystemService(Context.JOB_SCHEDULER_SERVICE);
        ComponentName componentName =
                new ComponentName(context, AwVariationsSeedFetchService.class);
        JobInfo.Builder builder =
                new JobInfo.Builder(TaskIds.WEBVIEW_VARIATIONS_SEED_FETCH_JOB_ID, componentName)
                        .setMinimumLatency(0)
                        .setOverrideDeadline(0)
                        .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY);
        int result = scheduler.schedule(builder.build());
        if (result != JobScheduler.RESULT_SUCCESS) {
            Log.e(TAG, "Failed to schedule the variations seed fetch job.");
        }
    }

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

    private static void sendMessageToWebView(Messenger msgr, int msgId) {
        if (msgr == null) {
            Log.e(TAG, "Reply Messenger not found, cannot send back data.");
            return;
        }
        Message msg = Message.obtain();
        msg.what = msgId;
        if (msgId == MSG_WITH_SEED_DATA) {
            try {
                Bundle bundle = new Bundle();
                bundle.putParcelable(KEY_SEED_DATA, getSeedFileDescriptor(SEED_DATA_FILENAME));
                bundle.putParcelable(KEY_SEED_PREF, getSeedFileDescriptor(SEED_PREF_FILENAME));
                msg.setData(bundle);
            } catch (IOException e) {
                Log.e(TAG, "Failed to read variations seed data. " + e);
            }
        }
        try {
            msgr.send(msg);
        } catch (RemoteException e) {
            Log.e(TAG, "Error passing service object back to WebView. " + e);
        }
    }

    /**
     * Store the variations seed data as base64 string into a file in a given directory.
     * @param seedData The seed data.
     * @param variationsDir The directory where the file stores into.
     */
    @VisibleForTesting
    public static void storeSeedDataToFile(byte[] seedData, File variationsDir) throws IOException {
        if (seedData == null) {
            Log.e(TAG, "Variations seed preference is null.");
            return;
        }
        BufferedWriter writer = null;
        try {
            sSeedDataLock.writeLock().lock();
            File seedDataTempFile = File.createTempFile(SEED_DATA_FILENAME, null, variationsDir);
            writer = new BufferedWriter(new FileWriter(seedDataTempFile));
            writer.write(Base64.encodeToString(seedData, Base64.NO_WRAP));
            renameTempFile(seedDataTempFile, new File(variationsDir, SEED_DATA_FILENAME));
        } finally {
            sSeedDataLock.writeLock().unlock();
            closeStream(writer);
        }
    }

    /**
     * Store the variations seed preference line by line as a file in a given directory.
     * @param seedPref The seed preference holds the related field of the seed.
     * @param variationsDir The directory where the file stores into.
     * @throws IOException if fail to store the seed preference into a file in the given directory.
     */
    @VisibleForTesting
    public static void storeSeedPrefToFile(SeedPreference seedPref, File variationsDir)
            throws IOException {
        if (seedPref == null) {
            Log.e(TAG, "Variations seed preference is null.");
            return;
        }
        List<String> seedPrefAsList = seedPref.toList();
        BufferedWriter writer = null;
        try {
            sSeedPrefLock.writeLock().lock();
            File seedPrefTempFile = File.createTempFile(SEED_PREF_FILENAME, null, variationsDir);
            writer = new BufferedWriter(new FileWriter(seedPrefTempFile));
            for (String s : seedPrefAsList) {
                writer.write(s);
                writer.newLine();
            }
            renameTempFile(seedPrefTempFile, new File(variationsDir, SEED_PREF_FILENAME));
        } finally {
            sSeedPrefLock.writeLock().unlock();
            closeStream(writer);
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
        File seedPrefFile = new File(variationsDir, SEED_PREF_FILENAME);
        try {
            sSeedPrefLock.readLock().lock();
            reader = new BufferedReader(new FileReader(seedPrefFile));
            ArrayList<String> seedPrefAsList = new ArrayList<String>();
            String line = null;
            while ((line = reader.readLine()) != null) {
                seedPrefAsList.add(line);
            }
            return SeedPreference.fromList(seedPrefAsList);
        } finally {
            sSeedPrefLock.readLock().unlock();
            closeStream(reader);
        }
    }

    /**
     * Get the file descriptor of the variations seed file in the service directory.
     * @return The parcel file descriptor of an opened seed file.
     * @throws IOException if fail to get or create the WebView variations directory.
     */
    @VisibleForTesting
    public static ParcelFileDescriptor getSeedFileDescriptor(String seedFile) throws IOException {
        File webViewVariationsDir = getOrCreateVariationsDirectory();
        return ParcelFileDescriptor.open(
                new File(webViewVariationsDir, seedFile), ParcelFileDescriptor.MODE_READ_ONLY);
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
            AwVariationsConfigurationService.renameTempFile(
                    tempFile, new File(variationsDir, fileName));
        } finally {
            lock.unlock();
            AwVariationsConfigurationService.closeStream(in);
            AwVariationsConfigurationService.closeStream(out);
            try {
                fileDescriptor.close();
            } catch (IOException e) {
                Log.e(TAG, "Failed to close seed data file descriptor. " + e);
            }
        }
    }

    private static ReentrantReadWriteLock getLockByName(String fileName) {
        if (SEED_DATA_FILENAME.equals(fileName)) {
            return sSeedDataLock;
        }
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
     * Get or create the variations directory in the service directory.
     * @return The variations directory path.
     * @throws IOException if fail to create the directory and get the directory.
     */
    @VisibleForTesting
    public static File getOrCreateVariationsDirectory() throws IOException {
        File webViewFileDir = ContextUtils.getApplicationContext().getFilesDir();
        File dir = new File(webViewFileDir, WEBVIEW_VARIATIONS_DIR);
        if (dir.mkdir() || dir.isDirectory()) {
            return dir;
        }
        throw new IOException("Failed to get or create the WebView variations directory.");
    }

    /**
     * Rename a temp file to a new name.
     * @param tempFile The temp file.
     * @param newFile The new file.
     */
    public static void renameTempFile(File tempFile, File newFile) {
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

    /**
     * Read the test seed data in the file.
     * @param variationsDir The given variations directory.
     * @return String holds the base64-encoded test seed data.
     * @throws IOException if fail to get or create the WebView variations directory.
     */
    @VisibleForTesting
    public static String getSeedDataForTesting(File variationsDir) throws IOException {
        BufferedReader reader = null;
        try {
            File file = new File(variationsDir, SEED_DATA_FILENAME);
            reader = new BufferedReader(new FileReader(file));
            return reader.readLine();
        } finally {
            closeStream(reader);
        }
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
