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
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.Closeable;
import java.io.File;
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

/**
 * AwVariationsConfigurationService is a bound service that shares the Finch seed with all the
 * WebViews on the system. When a WebView asks for the seed, it will immediately return the local
 * seed if it is not expired. Otherwise it will schedule a seed fetch job to the job service
 * and return message just to make the WebViews unbind from the
 * service. When multiple WebViews ask for the seed, will schedule several jobs, while on the job
 * service side, we will make the jobs run in the single-thread way and check if the seed is expired
 * before trying to fetch the seed, so to make sure every
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsConfigurationService extends Service {
    private static final String TAG = "AwVariatnsConfigSvc";

    // Types for the WebViews to identify the message. If it is MSG_WITH_SEED_DATA, the WebView will
    // store the seed data in the extra field of the message to the app directory.
    public static final int MSG_WITHOUT_SEED_DATA = 0;
    public static final int MSG_WITH_SEED_DATA = 1;

    // Types for the service to identify the message which Context is binding to it.
    public static final int MSG_SEED_REQUEST = 0;
    public static final int MSG_SEED_TRANSFER = 1;

    // The file names used to store the variations seed.
    public static final String SEED_DATA_FILENAME = "variations_seed_data";
    public static final String SEED_PREF_FILENAME = "variations_seed_pref";

    // The key string used in the extra field of the Message to store the variations seed.
    public static final String KEY_SEED_DATA = "KEY_SEED_DATA";
    public static final String KEY_SEED_PREF = "KEY_SEED_PREF";

    // The path name used to store the variations seed.
    public static final String WEBVIEW_VARIATIONS_DIR = "WebView_Variations";

    // The expiration time for the Finch seed.
    public static final long SEED_EXPIRATION_TIME_IN_MILLIS = TimeUnit.MINUTES.toMillis(30);

    // Messenger to handle seed requests from the WebView.
    private final Messenger mOnBindMessenger = new Messenger(new OnBindHandler());

    private static final Object sScheduleLock = new Object();

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

    // Handler classes should be static or leaks might occur.
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

    private static void handleSeedTransfer(Message msg) {
        Bundle data = msg.getData();
        byte[] seedData = data.getByteArray(KEY_SEED_DATA);
        ArrayList<String> seedPrefAsList = data.getStringArrayList(KEY_SEED_PREF);
        try {
            File webViewVariationsDir = getOrCreateVariationsDirectory();
            storeSeedDataToFile(seedData, webViewVariationsDir);
            storeSeedPrefToFile(SeedPreference.fromList(seedPrefAsList), webViewVariationsDir);
        } catch (IOException e) {
            Log.e(TAG, "Failed to write seed to the service directory. " + e);
        }
    }

    private static void handleSeedRequest(Message msg) {
        // TODO: add tests for this logic:
        // 1. There are only one seed fetch job once per time
        // 2. Send back empty message if the seed is expired
        // 2. Send back message with data if the seed is not expired
        synchronized (sScheduleLock) {
            if (isSeedExpiredInServiceDirectory()) {
                scheduleSeedFetchJob();
                sendMessageToWebView(msg.replyTo, MSG_WITHOUT_SEED_DATA);
            } else {
                sendMessageToWebView(msg.replyTo, MSG_WITH_SEED_DATA);
            }
        }
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
        // schedule() will stop the current running job which is not we want, while enqueue will
        // allow multiple jobs with the same job id, and we can make sure that only one job run per
        // time by checking if the seed is expired in the job service.
        int result = scheduler.schedule(builder.build());
        if (result != JobScheduler.RESULT_SUCCESS) {
            Log.e(TAG, "Failed to schedule the variations seed fetch job.");
        }
    }

    /**
     * Judge if the current Finch seed in the service directory is expired.
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
     * Judge if the current Finch seed in the service directory is expired.
     * @return True if the seed is expired or missing, otherwise false.
     */
    public static boolean isSeedExpiredInServiceDirectory() {
        try {
            return isSeedExpired(getOrCreateVariationsDirectory());
        } catch (IOException e) {
            Log.e(TAG, "Failed to check if the seed expired in the service directory. " + e);
            return true;
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
            File seedDataTempFile = File.createTempFile(SEED_DATA_FILENAME, null, variationsDir);
            writer = new BufferedWriter(new FileWriter(seedDataTempFile));
            writer.write(Base64.encodeToString(seedData, Base64.NO_WRAP));
            renameTempFile(seedDataTempFile, new File(variationsDir, SEED_DATA_FILENAME));
        } finally {
            closeStream(writer);
        }
    }

    /**
     * Store the variations seed preference line by line as a file in a given directory.
     * @param seedPref The seed preference holds the related field of the seed.
     * @param variationsDir The directory where the file stores into.
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
            File seedPrefTempFile = File.createTempFile(SEED_PREF_FILENAME, null, variationsDir);
            writer = new BufferedWriter(new FileWriter(seedPrefTempFile));
            for (String s : seedPrefAsList) {
                writer.write(s);
                writer.newLine();
            }
            renameTempFile(seedPrefTempFile, new File(variationsDir, SEED_PREF_FILENAME));
        } finally {
            closeStream(writer);
        }
    }

    /**
     * Get the variations seed preference in the app directory to an array list.
     * @return The array list holds the seed preference.
     */
    @VisibleForTesting
    public static SeedPreference getSeedPreference(File variationsDir) throws IOException {
        BufferedReader reader = null;
        File seedPrefFile = new File(variationsDir, SEED_PREF_FILENAME);
        try {
            reader = new BufferedReader(new FileReader(seedPrefFile));
            ArrayList<String> seedPrefAsList = new ArrayList<String>();
            String line = null;
            while ((line = reader.readLine()) != null) {
                seedPrefAsList.add(line);
            }
            return SeedPreference.fromList(seedPrefAsList);
        } finally {
            closeStream(reader);
        }
    }

    /**
     * SeedPreference is used to serialize/deserialize related fields of seed data when reading or
     * writing them into file.
     */
    public static class SeedPreference {
        public final String signature;
        public final String country;
        public final String date;
        public final boolean isGzipCompressed;
        public static final int FIELD_SIZE = 4;

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

    private static File getOrCreateVariationsDirectory() throws IOException {
        File webViewFileDir = ContextUtils.getApplicationContext().getFilesDir();
        File dir = new File(webViewFileDir, WEBVIEW_VARIATIONS_DIR);
        if (dir.mkdir() || dir.isDirectory()) {
            return dir;
        }
        throw new IOException("Failed to get or create the WebView variations directory.");
    }

    /**
     * Get the file descriptor of the variations seed file from the service directory.
     * @return The parcel file directory of an opened seed file.
     * @throws IOException if fail to get or create the WebView Variations directory.
     */
    @VisibleForTesting
    public static ParcelFileDescriptor getSeedFileDescriptor(String seedFile) throws IOException {
        File webViewVariationsDir = getOrCreateVariationsDirectory();
        return ParcelFileDescriptor.open(
                new File(webViewVariationsDir, seedFile), ParcelFileDescriptor.MODE_READ_ONLY);
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
                    "Failed to rename file " + tempFile.getAbsolutePath() + " to "
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
     * Clear the test data.
     * @throws IOException if fail to get or create the WebView Variations directory.
     */
    @SuppressFBWarnings("RV_RETURN_VALUE_IGNORED_BAD_PRACTICE") // ignoring File.delete() return
    @VisibleForTesting
    public static void clearDataForTesting() throws Exception {
        File appWebViewDir = new File(PathUtils.getDataDirectory());
        FileUtils.recursivelyDeleteFile(appWebViewDir);
    }
}
