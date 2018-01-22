// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import org.chromium.base.Log;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.SequenceInputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.Charset;
import java.nio.charset.UnsupportedCharsetException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.nio.charset.Charset;
import java.nio.charset.UnsupportedCharsetException;
import java.util.Collections;

/**
 * Crash crashdump uploader. Scans the crash dump location provided by CastCrashReporterClient for
 * dump files, attempting to upload all crash dumps to the crash server.
 *
 * <p>Uploading is intended to happen in a background thread, and this method will likely be called
 * on startup, looking for crash dumps from previous runs, since Chromium's crash code explicitly
 * blocks any post-dump hooks or uploading for Android builds.
 */
public final class CastCrashUploader {
    private static final String TAG = "cr_CastCrashUploader";
    private static final String CRASH_REPORT_HOST = "clients2.google.com";
    private static final String CAST_SHELL_USER_AGENT = android.os.Build.MODEL + "/CastShell";
    // Multipart dump filename has format "[random string].dmp[pid]", e.g.
    // 20597a65-b822-008e-31f8fc8e-02bb45c0.dmp18169
    private static final String DUMP_FILE_REGEX = ".*\\.dmp\\d*";

    private final ScheduledExecutorService mExecutorService;
    private final String mCrashDumpPath;
    private final String mCrashReportUploadUrl;
    private final String mUuid;
    private final String mApplicationFeedback;
    private final Runnable mQueueAllCrashDumpUploadsRunnable =
            () -> queueAllCrashDumpUploads(false);

    public CastCrashUploader(ScheduledExecutorService executorService, String crashDumpPath,
            String uuid, String applicationFeedback, boolean uploadCrashToStaging) {
        mExecutorService = executorService;
        mCrashDumpPath = crashDumpPath;
        mUuid = uuid;
        mApplicationFeedback = applicationFeedback;

        mCrashReportUploadUrl = uploadCrashToStaging
                ? "https://clients2.google.com/cr/staging_report"
                : "https://clients2.google.com/cr/report";
    }

    /** Sets up a periodic uploader, that checks for new dumps to upload every 20 minutes */
    @SuppressWarnings("FutureReturnValueIgnored")
    public void startPeriodicUpload() {
        mExecutorService.scheduleWithFixedDelay(mQueueAllCrashDumpUploadsRunnable,
                0, // Do first run immediately
                20, // Run once every 20 minutes
                TimeUnit.MINUTES);
    }

    @SuppressWarnings("FutureReturnValueIgnored")
    public void uploadOnce() {
        mExecutorService.schedule(mQueueAllCrashDumpUploadsRunnable, 0, TimeUnit.MINUTES);
    }

    public void removeCrashDumps() {
        File crashDumpDirectory = new File(mCrashDumpPath);
        for (File potentialDump : crashDumpDirectory.listFiles()) {
            if (potentialDump.getName().matches(DUMP_FILE_REGEX)) {
                potentialDump.delete();
            }
        }
    }

    /**
     * Searches for files matching the given regex in the crash dump folder, queueing each one for
     * upload.
     *
     * @param synchronous Whether or not this function should block on queued uploads
     */
    private void queueAllCrashDumpUploads(boolean synchronous) {
        if (mCrashDumpPath == null) return;
        Log.i(TAG, "Checking for crash dumps");

        List<Future> tasks = new ArrayList<Future>();
        File crashDumpDirectory = new File(mCrashDumpPath);

        for (final File potentialDump : crashDumpDirectory.listFiles()) {
            String dumpName = potentialDump.getName();
            if (dumpName.matches(DUMP_FILE_REGEX)) {
                DumpStream.DumpStreamBuilder dumpStreamBuilder = new DumpStream.DumpStreamBuilder(
                        mUuid, mApplicationFeedback, potentialDump);
                DumpStream dumpStream = dumpStreamBuilder.constructDumpStream();

                tasks.add(mExecutorService.submit(() -> uploadCrashDump(dumpStream)));
            }
        }

        // Wait on tasks, if necessary.
        if (synchronous) {
            for (Future task : tasks) {
                // Wait on task. If thread received interrupt and should stop waiting, return.
                if (!waitOnTask(task)) {
                    return;
                }
            }
        }
    }

    private void uploadCrashDump(DumpStream dumpStream) {
        try {
            HttpURLConnection connection =
                    (HttpURLConnection) new URL(mCrashReportUploadUrl).openConnection();

            // Expect a report ID as the entire response
            try {
                connection.setDoOutput(true);
                connection.setRequestProperty("Content-Type",
                        "multipart/form-data; boundary=" + dumpStream.getMimeBoundary());

                streamCopy(dumpStream.getStream(), connection.getOutputStream());

                String responseLine =
                        DumpStream.DumpStreamBuilder.getFirstLine(connection.getInputStream());

                int responseCode = connection.getResponseCode();
                if (responseCode == HttpURLConnection.HTTP_OK
                        || responseCode == HttpURLConnection.HTTP_CREATED
                        || responseCode == HttpURLConnection.HTTP_ACCEPTED) {
                    Log.i(TAG, "Successfully uploaded to %s, report ID: %s", mCrashReportUploadUrl,
                            responseLine);
                } else {
                    Log.e(TAG, "Failed response (%d): %s", responseCode,
                            connection.getResponseMessage());

                    // 400 Bad Request is returned if the dump file is malformed. If request
                    // is not malformed, short-circuit before cleanup to avoid deletion and
                    // retry later, otherwise pass through and delete malformed file.
                    if (responseCode != HttpURLConnection.HTTP_BAD_REQUEST) {
                        return;
                    }
                }
            } catch (FileNotFoundException fnfe) {
                // Android's HttpURLConnection implementation fires FNFE on some errors.
                Log.e(TAG, "Failed response: " + connection.getResponseCode(), fnfe);
            } catch (UnsupportedCharsetException e) {
                Log.wtf(TAG, "UTF-8 not supported");
            } finally {
                connection.disconnect();
            }
            dumpStream.close();
        } catch (IOException e) {
            Log.e(TAG, "Error occurred trying to upload crash dump", e);
        }
    }

    /**
     * Copies all available data from |inStream| to |outStream|. Closes both streams when done.
     *
     * @param inStream the stream to read
     * @param outStream the stream to write to
     * @throws IOException
     */
    private static void streamCopy(InputStream inStream, OutputStream outStream)
            throws IOException {
        byte[] temp = new byte[4096];
        int bytesRead = inStream.read(temp);
        while (bytesRead >= 0) {
            outStream.write(temp, 0, bytesRead);
            bytesRead = inStream.read(temp);
        }
        inStream.close();
        outStream.close();
    }

    /**
     * Waits until Future is propagated
     *
     * @return Whether thread should continue waiting
     */
    private boolean waitOnTask(Future task) {
        try {
            task.get();
            return true;
        } catch (InterruptedException e) {
            // Was interrupted while waiting, tell caller to cancel waiting
            return false;
        } catch (ExecutionException e) {
            // Task execution may have failed, but this is fine as long as it finished
            // executing
            return true;
        }
    }
}
