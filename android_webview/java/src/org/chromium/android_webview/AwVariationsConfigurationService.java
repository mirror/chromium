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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
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
 * and return the new seed after the seed fetch completes. The service will make sure that there
 * will be only one fetch job scheduled at a time.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP) // JobService requires API level 21.
public class AwVariationsConfigurationService extends Service {
    private static final String TAG = "AwVariatnsConfigSvc";

    public static final String SEED_DATA = "seed_data";
    public static final String SEED_PREF = "seed_pref";
    public static final String SEED_FETCH_COMPLETE_NOTIFIER_INTENT_KEY =
            "SeedFetchCompleteNotifierIntentKey";

    private static final int BUFFER_SIZE = 4096;
    private static final long SEED_EXPIRATION_TIME = 30 * 60 * 1000;
    private static final long SEED_FETCH_TIMEOUT = 5 * 1000;

    @Override
    public void onCreate() {
        super.onCreate();
        ContextUtils.initApplicationContext(this.getApplicationContext());
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mSeedRequestMessenger.getBinder();
    }

    private final Messenger mSeedRequestMessenger = new Messenger(new SeedRequestHandler(this));

    private static class SeedRequestHandler extends Handler {
        private AwVariationsConfigurationService mService;

        SeedRequestHandler(AwVariationsConfigurationService svc) {
            mService = svc;
        }

        @Override
        public void handleMessage(Message msg) {
            new SeedRequestTask(msg.replyTo, mService)
                    .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    };

    private static class SeedRequestTask extends AsyncTask<Void, Void, Void> {
        private Messenger mMessenger;
        private AwVariationsConfigurationService mService;

        SeedRequestTask(Messenger msgr, AwVariationsConfigurationService svc) {
            mMessenger = msgr;
            mService = svc;
        }

        @Override
        protected Void doInBackground(Void... params) {
            mService.handleSeedRequest(mMessenger);
            return null;
        }
    }

    private void handleSeedRequest(Messenger mMessenger) {
        if (!waitUtilSeedNotExpired()) {
            Log.e(TAG, "Failed to wait until the seed refresh.");
            return;
        }
        Log.e(TAG, "send back seed");
        sendVariationsSeed(mMessenger);
    }

    private static boolean sIsSchedulingJob = false;
    private static final Object sScheduleLock = new Object();

    private boolean waitUtilSeedNotExpired() {
        synchronized (sScheduleLock) {
            if (!sIsSchedulingJob && isSeedExpired()) {
                scheduleSeedFetchJob();
                sIsSchedulingJob = true;
            }
            Log.e(TAG, "waiting seed to be fetched");
            while (isSeedExpired()) {
                try {
                    sScheduleLock.wait(SEED_FETCH_TIMEOUT);
                    Log.e(TAG, "waiting seed to be fetched");
                } catch (InterruptedException e) {
                    Log.e(TAG, "The waiting thread was interrupted by another thread. " + e);
                    return false;
                }
            }
            sIsSchedulingJob = false;
            return true;
        }
    }

    private final Messenger mSeedFetchCompleteNotifier =
            new Messenger(new SeedFetchCompleteNotifyHandler(sScheduleLock));

    private static class SeedFetchCompleteNotifyHandler extends Handler {
        private Object mScheduleLock;

        SeedFetchCompleteNotifyHandler(Object lock) {
            mScheduleLock = lock;
        }

        @Override
        public void handleMessage(Message msg) {
            synchronized (mScheduleLock) {
                Log.e(TAG, "Seed Fetch Complete notified by messenger.");
                mScheduleLock.notifyAll();
            }
        }
    };

    private void scheduleSeedFetchJob() {
        Log.e(TAG, "scheduling job");
        Intent startServiceIntent = new Intent(this, AwVariationsSeedFetchService.class);
        startServiceIntent.putExtra(
                SEED_FETCH_COMPLETE_NOTIFIER_INTENT_KEY, mSeedFetchCompleteNotifier);
        startService(startServiceIntent);

        JobScheduler scheduler = (JobScheduler) getSystemService(Context.JOB_SCHEDULER_SERVICE);
        ComponentName componentName = new ComponentName(this, AwVariationsSeedFetchService.class);
        JobInfo.Builder builder = new JobInfo.Builder(0, componentName).setMinimumLatency(0);
        builder.setOverrideDeadline(0);
        builder.setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY);
        int result = scheduler.schedule(builder.build());
        assert result == JobScheduler.RESULT_SUCCESS;
    }

    private static void sendVariationsSeed(Messenger msgr) {
        if (msgr == null) {
            Log.e(TAG, "Reply Messenger not found, cannot send back data.");
            return;
        }
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
        return new Date().getTime() - lastSeedFetchTime.getTime() > SEED_EXPIRATION_TIME;
    }

    private static Date getLastSeedFetchTime() {
        Date lastSeedFetchTime = null;
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
    public static void bindService(String webViewPackageName, Context context) {
        final Intent intent = new Intent();
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

        if (!context.bindService(intent, connection, Context.BIND_AUTO_CREATE)) {
            Log.w(TAG, "Could not bind to AwVariationsConfigurationService. " + intent);
        }
    }

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
            Log.e(TAG, "Seed Pref signature: " + seedPrefList.get(0));
            Log.e(TAG, "Seed Pref country:   " + seedPrefList.get(1));
            Log.e(TAG, "Seed Pref date:      " + seedPrefList.get(2));
            Log.e(TAG, "Seed Pref compress:  " + seedPrefList.get(3));

            final Context appContext = ContextUtils.getApplicationContext();
            appContext.unbindService(mConnection);
            File appFilesDir = appContext.getFilesDir();
            Log.e(TAG, "App files dir: " + appFilesDir.getAbsolutePath());
            storeSeedDataInAppDir(appFilesDir, seedDataFileDescriptor);
            storeSeedPrefInAppDir(appFilesDir, seedPrefList);
        }
    }

    private static File getOrCreateWebViewVariationsDirInAppDir(File appDir) throws IOException {
        File dir = new File(appDir, WEBVIEW_VARIATIONS_DIR);
        if (dir.mkdir() || dir.isDirectory()) {
            return dir;
        }
        throw new IOException("Failed to get or create the WebView variations dir in app dir.");
    }

    private static void storeSeedDataInAppDir(
            File appDir, ParcelFileDescriptor seedDataFileDescriptor) {
        FileInputStream in = null;
        FileOutputStream out = null;
        try {
            File webViewVariationsDir = getOrCreateWebViewVariationsDirInAppDir(appDir);
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
            Log.e(TAG, "Failed to copy variations seed data to the WebView host app." + e);
        } finally {
            AwVariationsSeedFetchService.closeStream(in);
            AwVariationsSeedFetchService.closeStream(out);
        }
    }

    private static void storeSeedPrefInAppDir(File appDir, ArrayList<String> seedPrefList) {
        FileWriter out = null;
        try {
            File webViewVariationsDir = getOrCreateWebViewVariationsDirInAppDir(appDir);
            File seedPrefTempFile =
                    File.createTempFile(SEED_PREF_FILENAME, null, webViewVariationsDir);
            out = new FileWriter(seedPrefTempFile);
            for (String s : seedPrefList) {
                out.write(s);
            }
            AwVariationsSeedFetchService.renameTempFile(
                    seedPrefTempFile, new File(webViewVariationsDir, SEED_PREF_FILENAME));
        } catch (IOException e) {
            Log.e(TAG, "Failed to write variations seed pref to the WebView host app." + e);
        } finally {
            AwVariationsSeedFetchService.closeStream(out);
        }
    }
}
