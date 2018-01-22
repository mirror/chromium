// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import org.chromium.base.Log;
import org.chromium.base.VisibleForTesting;

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
import java.util.ArrayList;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.nio.charset.Charset;
import java.nio.charset.UnsupportedCharsetException;
import java.util.Collections;

/**
 * Handles creation of stream for uploading of crash dump
 * along with appending other
 *
 */
public final class DumpStreamConstructor {
    private static final String TAG = "cr_DumpStreamConstructor";

    private final String mUuid;
    private final String mApplicationFeedback;
    private final String mLogcat;
    private final File mMinidumpFile;
    private final String mDumpFirstLine;
    private final String mMimeBoundary;

    @VisibleForTesting
    public final ArrayList<InputStream> v;

    public DumpStreamConstructor(
            String uuid, String applicationFeedback, String logcat, File minidumpFile) {
        mUuid = uuid;
        mApplicationFeedback = applicationFeedback;
        mLogcat = logcat;
        mMinidumpFile = minidumpFile;
        mDumpFirstLine = extractDumpFirstLine();
        mMimeBoundary = mDumpFirstLine.substring(2);
        v = new ArrayList<>();
    }

    public DumpStreamConstructor(String uuid, String applicationFeedback, File minidumpFile) {
        this(uuid, applicationFeedback, getLogs(), minidumpFile);
    }

    public String getMimeBoundary() {
        return mMimeBoundary;
    }

    public void constructDumpStream() {
        try {
            addAppFeedbackToStream();
            addUuidToStream();
            addLogcatToStream();
            addMinidumpToStream();
        } catch (UnsupportedCharsetException e) {
            // Should never happen; all Java implementations are required to support UTF-8.
            Log.wtf(TAG, "UTF-8 not supported", e);
        }
    }

    public InputStream getStream() {
        return new SequenceInputStream(Collections.enumeration(v));
    }

    public void deleteMinidump() throws SecurityException {
        mMinidumpFile.delete();
    }

    @VisibleForTesting
    public static String getLogs() {
        String log = "";
        try {
            Log.i(TAG, "Getting logcat");
            log = LogcatExtractor.getElidedLogcat();
            Log.d(TAG, "Log size" + log.length());
            return log;

        } catch (IOException | InterruptedException e) {
            Log.e(TAG, "Getting logcat failed ", e);
        }
        return "";
    }

    @VisibleForTesting
    public void addUuidToStream() {
        if (!mUuid.equals("")) {
            Log.i(TAG, "Including UUID: " + mUuid);
            v.add(new ByteArrayInputStream(constructUuid().getBytes(Charset.forName("UTF-8"))));
        } else {
            Log.d(TAG, "No UUID");
        }
    }

    @VisibleForTesting
    public void addAppFeedbackToStream() {
        if (!mApplicationFeedback.equals("")) {
            Log.i(TAG, "Including feedback");
            v.add(new ByteArrayInputStream(
                    constructAppFeedback().getBytes(Charset.forName("UTF-8"))));
        } else {
            Log.d(TAG, "No Feedback");
        }
    }
    @VisibleForTesting
    public void addLogcatToStream() {
        if (!mLogcat.equals("")) {
            Log.i(TAG, "Including log file");
            v.add(new ByteArrayInputStream(constructLogcat().getBytes(Charset.forName("UTF-8"))));
        } else {
            Log.d(TAG, "No Logcat");
        }
    }

    @VisibleForTesting
    public void addMinidumpToStream() {
        try {
            v.add(new FileInputStream(mMinidumpFile));
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Minidump not found", e);
        }
    }

    @VisibleForTesting
    public String constructUuid() {
        StringBuilder uuidBuilder = new StringBuilder();
        uuidBuilder.append(mDumpFirstLine);
        uuidBuilder.append("\n");
        uuidBuilder.append(generateTextFieldMineHeader("comments"));
        uuidBuilder.append(mUuid);
        uuidBuilder.append("\n");
        return uuidBuilder.toString();
    }

    @VisibleForTesting
    public String constructAppFeedback() {
        Log.i(TAG, "Including feedback");
        StringBuilder feedbackHeader = new StringBuilder();
        feedbackHeader.append(mDumpFirstLine);
        feedbackHeader.append("\n");
        feedbackHeader.append(
                generateFileMimeHeader("application_feedback.txt", "application.txt"));
        feedbackHeader.append(mApplicationFeedback);
        feedbackHeader.append("\n");
        return feedbackHeader.toString();
    }

    @VisibleForTesting
    public String constructLogcat() {
        StringBuilder logHeader = new StringBuilder();
        logHeader.append(mDumpFirstLine);
        logHeader.append("\n");
        logHeader.append(generateFileMimeHeader("log.txt", "log.txt"));
        logHeader.append(mLogcat);
        logHeader.append("\n");
        return logHeader.toString();
    }

    // Dump file is already in multipart MIME format and has a boundary throughout.
    // Scrape the first line, remove two dashes, call that the "boundary" and add it
    // to the content-type.
    @VisibleForTesting
    public String extractDumpFirstLine() {
        String dumpFirstLine = "";
        try (FileInputStream dumpFileStream = new FileInputStream(mMinidumpFile)) {
            dumpFirstLine = getFirstLine(dumpFileStream);
        } catch (IOException e) {
            Log.e(TAG, "Error occurred trying to parse crash dump mimeBoundary", e);
        }
        return dumpFirstLine;
    }

    // header for MIME multipart text field
    @VisibleForTesting
    public static String generateTextFieldMineHeader(String name) {
        return "Content-Disposition: form-data; name=\"" + name + "\"\n"
                + "Content-Type: text/plain\n\n";
    }

    // header for MIME multipart fitle field
    @VisibleForTesting
    public static String generateFileMimeHeader(String name, String filename) {
        return "Content-Disposition: form-data; name=\"" + name + "\"; filename=\"" + filename
                + "\"\nContent-Type: text/plain\n\n";
    }

    /**
     * Gets the first line from an input stream
     *
     * @return First line of the input stream.
     * @throws IOException
     */
    public static String getFirstLine(InputStream inputStream) throws IOException {
        try (InputStreamReader streamReader =
                        new InputStreamReader(inputStream, Charset.forName("UTF-8"));
                BufferedReader reader = new BufferedReader(streamReader)) {
            return reader.readLine();
        } catch (UnsupportedCharsetException e) {
            // Should never happen; all Java implementations are required to support UTF-8.
            Log.wtf(TAG, "UTF-8 not supported", e);
            return "";
        }
    }
}
