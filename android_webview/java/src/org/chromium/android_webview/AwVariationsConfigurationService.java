// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import static org.chromium.android_webview.AwVariationsSeedFetchService.SEED_DATA_FILENAME;
import static org.chromium.android_webview.AwVariationsSeedFetchService.SEED_PREF_FILENAME;
import static org.chromium.android_webview.AwVariationsSeedFetchService.WEBVIEW_VARIATIONS_DIR;

import android.annotation.TargetApi;
import android.app.Service;
import android.app.job.JobInfo;
import android.app.job.JobScheduler;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;

import org.chromium.android_webview.AwVariationsSeedFetchService.SeedPreference;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

/**
 * AwVariationsConfigurationService is a bound servicee to share Finch seed with all the WebViews on
 * the system. When a WebView asks for seed, it will immediately return the existing seed if it is
 * not expired. Otherwise it will fetch the seed by scheduling a job to AwVariationsSeedFetchService
 * and return the new seed after the seed fetch completes. When multiple WebViews ask for the seed,
 * sScheduleLock will send one seed fetch job when the first comes while block the others and put
 * them into wait state. The job service will used the sSeedFetchCompleteNotifier to notify the
 * AwVariationsConfigurationService that seed fetch complete. It will wake the wait threads and let
 * them send the new seed back to the WebViews. The tryLock mechanism on the job service side is
 * just to make sure that no seed fetch job will be scheduled when there is one still running.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsConfigurationService extends Service {
    private static final String TAG = "AwVariatnsConfigSvc";

    public static final String SEED_DATA = "seed_data";
    public static final String SEED_PREF = "seed_pref";
    public static final String SEED_FETCH_COMPLETE_NOTIFIER_INTENT_KEY =
            "SeedFetchCompleteNotifierIntentKey";

    private static final int BUFFER_SIZE = 4096;
    private static final int SEED_FETCH_JOB_ID = 0;
    private static final long SEED_EXPIRATION_TIME_IN_MILLIS = 30 * 60 * 1000;
    private static final long SEED_FETCH_TIMEOUT_IN_MILLIS = 5 * 1000;

    private final Messenger mSeedRequestMessenger = new Messenger(new SeedRequestHandler());

    private static boolean sIsJobRunning = false;
    private static final Object sScheduleLock = new Object();

    private static final Messenger sSeedFetchCompleteNotifier =
            new Messenger(new SeedFetchCompleteNotifyHandler(sScheduleLock));

    @Override
    public void onCreate() {
        super.onCreate();
        // Ensure that ContextUtils.getApplicationContext() will return the current context in the
        // following code for the function won't work when we use the standalone WebView package.
        ContextUtils.initApplicationContext(this.getApplicationContext());
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mSeedRequestMessenger.getBinder();
    }

    // Handler classes should be static or leaks might occur.
    private static class SeedRequestHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            new SeedRequestTask(msg.replyTo).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    };

    private static class SeedRequestTask extends AsyncTask<Void, Void, Void> {
        private Messenger mMessenger;

        SeedRequestTask(Messenger msgr) {
            mMessenger = msgr;
        }

        @Override
        protected Void doInBackground(Void... params) {
            AwVariationsConfigurationService.handleSeedRequest(mMessenger);
            return null;
        }
    }

    private static void handleSeedRequest(Messenger mMessenger) {
        if (!waitUntilSeedNotExpired()) {
            Log.e(TAG, "Interrupted unexpectedly, failed to wait until the seed refresh.");
            return;
        }
        sendVariationsSeed(mMessenger);
    }

    private static boolean waitUntilSeedNotExpired() {
        synchronized (sScheduleLock) {
            // Schedule a seed fetch job only when the seed is expired and no job is processing.
            if (!sIsJobRunning && isSeedExpired()) {
                scheduleSeedFetchJob();
                sIsJobRunning = true;
            }
            while (isSeedExpired()) {
                try {
                    sScheduleLock.wait(SEED_FETCH_TIMEOUT_IN_MILLIS);
                } catch (InterruptedException e) {
                    Log.e(TAG, "Interrupted while waiting for variations seed. " + e);
                    return false;
                }
            }
            sIsJobRunning = false;
            return true;
        }
    }

    /**
     * The Handler class is used to notify all the seed request thread to get the seed back to the
     * WebViews. The handler is sent to the AwVariationsSeedFetchService and called when seed fetch
     * job is finished.
     * Handler classes should be static or leaks might occur.
     */
    private static class SeedFetchCompleteNotifyHandler extends Handler {
        private Object mScheduleLock;

        SeedFetchCompleteNotifyHandler(Object lock) {
            mScheduleLock = lock;
        }

        @Override
        public void handleMessage(Message msg) {
            synchronized (mScheduleLock) {
                mScheduleLock.notifyAll();
            }
        }
    };

    private static void scheduleSeedFetchJob() {
        Context context = ContextUtils.getApplicationContext();
        Intent startServiceIntent = new Intent(context, AwVariationsSeedFetchService.class);
        startServiceIntent.putExtra(
                SEED_FETCH_COMPLETE_NOTIFIER_INTENT_KEY, sSeedFetchCompleteNotifier);
        context.startService(startServiceIntent);

        JobScheduler scheduler =
                (JobScheduler) context.getSystemService(Context.JOB_SCHEDULER_SERVICE);
        ComponentName componentName =
                new ComponentName(context, AwVariationsSeedFetchService.class);
        JobInfo.Builder builder = new JobInfo.Builder(SEED_FETCH_JOB_ID, componentName)
                                          .setMinimumLatency(0)
                                          .setOverrideDeadline(0)
                                          .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY);
        int result = scheduler.schedule(builder.build());
        assert result == JobScheduler.RESULT_SUCCESS;
    }

    private static void sendVariationsSeed(Messenger msgr) {
        if (msgr == null) {
            Log.e(TAG, "Reply Messenger not found, cannot send back data.");
            return;
        }
        // No need to add job description for only one kind of job is going to be scheduled.
        Message m = Message.obtain();
        Bundle bundle = new Bundle();
        try {
            bundle.putParcelable(
                    SEED_DATA, AwVariationsSeedFetchService.getVariationsSeedDataFileDescriptor());
            bundle.putStringArrayList(SEED_PREF,
                    AwVariationsSeedFetchService.getVariationsSeedPreference().toArrayList());
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "Failed to find the class of the serialized object. " + e);
        } catch (IOException e) {
            Log.e(TAG, "Failed to read variations seed data. " + e);
        }
        m.setData(bundle);
        try {
            msgr.send(m);
        } catch (RemoteException e) {
            Log.e(TAG, "Error passing service object back to WebView. " + e);
        }
    }

    private static boolean isSeedExpired() {
        Date lastSeedFetchTime = getLastSeedFetchTime();
        if (lastSeedFetchTime == null) {
            return true;
        }
        return new Date().getTime() - lastSeedFetchTime.getTime() > SEED_EXPIRATION_TIME_IN_MILLIS;
    }

    private static Date getLastSeedFetchTime() {
        Date lastSeedFetchTime = null;
        // The date format comes from the date field in the HTTP header of the seed.
        SimpleDateFormat formatter = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z", Locale.US);
        try {
            SeedPreference seedPref = AwVariationsSeedFetchService.getVariationsSeedPreference();
            lastSeedFetchTime = formatter.parse(seedPref.date);
        } catch (ParseException e) {
            Log.e(TAG, "Failed to parse the last seed fetch time. " + e);
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "Failed to find the class of the serialized object. " + e);
        } catch (IOException e) {
            Log.e(TAG, "Failed to read variations seed preference. " + e);
        }
        return lastSeedFetchTime;
    }

    /**
     * Bind to AwVariationsConfigurationService in the current context for WebView package.
     * @params webViewPackageName The package name for the Android WebView.
     * @params context The current host application context.
     */
    public static void bindToVariationsService() {
        Intent intent = new Intent();
        String webViewPackageName = AwBrowserProcess.getWebViewPackageName();
        intent.setClassName(webViewPackageName, AwVariationsConfigurationService.class.getName());
        ServiceConnection connection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                Message message = Message.obtain();
                message.replyTo = new Messenger(new SeedDataReceiver(this));
                try {
                    new Messenger(service).send(message);
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

    /**
     * The class is used to received the seed data from the AwVariationsConfigurationService on the
     * WebView side. It will store the both seed data and preference as files into app directory.
     * Handler cannot be create as a non-static class because it may leak.
     */
    private static class SeedDataReceiver extends Handler {
        private ServiceConnection mConnection;

        SeedDataReceiver(ServiceConnection connection) {
            mConnection = connection;
        }

        @Override
        public void handleMessage(Message msg) {
            Bundle data = msg.getData();
            ParcelFileDescriptor seedDataFileDescriptor = data.getParcelable(SEED_DATA);
            ArrayList<String> seedPrefList = data.getStringArrayList(SEED_PREF);

            final Context appContext = ContextUtils.getApplicationContext();
            appContext.unbindService(mConnection);
            File appFilesDir = appContext.getFilesDir();
            storeSeedDataInAppDir(appFilesDir, seedDataFileDescriptor);
            storeSeedPrefInAppDir(appFilesDir, seedPrefList);
        }
    }

    private static File getOrCreateVariationsDir(File appDir) throws IOException {
        File dir = new File(appDir, WEBVIEW_VARIATIONS_DIR);
        if (dir.mkdir() || dir.isDirectory()) {
            return dir;
        }
        throw new IOException("Failed to get or create the WebView variations dir in app dir.");
    }

    /**
     * Store the variations seed data as a file in the app directory.
     * @param appDir The path of the app directory.
     * @param seedDataFileDescriptor The file descriptor of the opened seed data file in the service
     * directory.
     */
    @VisibleForTesting
    public static void storeSeedDataInAppDir(
            File appDir, ParcelFileDescriptor seedDataFileDescriptor) {
        if (seedDataFileDescriptor == null) {
            Log.e(TAG, "Variations seed data file descriptor is null.");
            return;
        }
        FileInputStream in = null;
        FileOutputStream out = null;
        try {
            File webViewVariationsDir = getOrCreateVariationsDir(appDir);
            Log.e(TAG, webViewVariationsDir.getAbsolutePath());
            File seedDataTempFile =
                    File.createTempFile(SEED_DATA_FILENAME, null, webViewVariationsDir);
            in = new FileInputStream(seedDataFileDescriptor.getFileDescriptor());
            out = new FileOutputStream(seedDataTempFile);
            byte[] buffer = new byte[BUFFER_SIZE];
            int readCount = 0;
            while ((readCount = in.read(buffer)) > 0) {
                out.write(buffer, 0, readCount);
            }
            AwVariationsSeedFetchService.renameTempFile(
                    seedDataTempFile, new File(webViewVariationsDir, SEED_DATA_FILENAME));
        } catch (IOException e) {
            Log.e(TAG, "Failed to copy variations seed data to file in the app." + e);
        } finally {
            AwVariationsSeedFetchService.closeStream(in);
            AwVariationsSeedFetchService.closeStream(out);
            try {
                seedDataFileDescriptor.close();
            } catch (IOException e) {
                Log.e(TAG, "Failed to close seed data file descriptor. " + e);
            }
        }
    }

    /**
     * Get the variations seed data in the app directory to an byte array.
     * @params appDir The path of the app directory.
     * @return The byte array holds the seed data.
     */
    @VisibleForTesting
    public static byte[] getSeedDataInAppDir(File appDir) throws IOException {
        FileInputStream inputStream = null;
        try {
            File webViewVariationsDir = getOrCreateVariationsDir(appDir);
            inputStream = new FileInputStream(new File(webViewVariationsDir, SEED_DATA_FILENAME));
            return VariationsSeedFetcher.convertInputStreamToByteArray(inputStream);
        } finally {
            AwVariationsSeedFetchService.closeStream(inputStream);
        }
    }

    /**
     * Store the variations seed preference line by line as a file in the app directory.
     * @param appDir The path of the app directory.
     * @param seedPrefAsList The array list which contains the seed preference.
     */
    @VisibleForTesting
    public static void storeSeedPrefInAppDir(File appDir, ArrayList<String> seedPrefAsList) {
        if (seedPrefAsList == null) {
            Log.e(TAG, "Variations seed preference is null.");
            return;
        }
        BufferedWriter writer = null;
        try {
            File webViewVariationsDir = getOrCreateVariationsDir(appDir);
            File seedPrefTempFile =
                    File.createTempFile(SEED_PREF_FILENAME, null, webViewVariationsDir);
            writer = new BufferedWriter(new FileWriter(seedPrefTempFile));
            for (String s : seedPrefAsList) {
                writer.write(s);
                writer.newLine();
            }
            AwVariationsSeedFetchService.renameTempFile(
                    seedPrefTempFile, new File(webViewVariationsDir, SEED_PREF_FILENAME));
        } catch (IOException e) {
            Log.e(TAG, "Failed to write variations seed pref to file in the app." + e);
        } finally {
            AwVariationsSeedFetchService.closeStream(writer);
        }
    }

    /**
     * Get the variations seed preference in the app directory to an array list.
     * @params appDir The path of the app directory.
     * @return The array list holds the seed preference.
     */
    @VisibleForTesting
    public static ArrayList<String> getSeedPrefInAppDir(File appDir) throws IOException {
        BufferedReader reader = null;
        try {
            File webViewVariationsDir = getOrCreateVariationsDir(appDir);
            reader = new BufferedReader(
                    new FileReader(new File(webViewVariationsDir, SEED_PREF_FILENAME)));
            ArrayList<String> seedPrefAsList = new ArrayList<String>();
            String line = null;
            while ((line = reader.readLine()) != null) {
                seedPrefAsList.add(line);
            }
            assert seedPrefAsList.size() == SeedPreference.FIELD_SIZE;
            return seedPrefAsList;
        } finally {
            AwVariationsSeedFetchService.closeStream(reader);
        }
    }

    /**
     * Clear the test data.
     * @throws IOException if fail to get or create the WebView Variations directory.
     */
    @SuppressFBWarnings("RV_RETURN_VALUE_IGNORED_BAD_PRACTICE") // ignoring File.delete() return
    @VisibleForTesting
    public static void clearDataForTesting(File appDir) throws IOException {
        File webViewVariationsDir = getOrCreateVariationsDir(appDir);
        File seedDataFile = new File(webViewVariationsDir, SEED_DATA_FILENAME);
        File seedPrefFile = new File(webViewVariationsDir, SEED_PREF_FILENAME);
        seedDataFile.delete();
        seedPrefFile.delete();
        webViewVariationsDir.delete();
    }
}
