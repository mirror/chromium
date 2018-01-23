// Copyright 2018 The Chromium Authors. All rights reserved.
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
public final class DumpStream {
    private static final String TAG = "cr_DumpStream";

    private final File mMinidumpFile;
    private final String mMimeBoundary;
    private final InputStream mDumpStream;

    private DumpStream(File minidumpFile, String mimeBoundary, InputStream dumpStream) {
        mMinidumpFile = minidumpFile;
        mMimeBoundary = mimeBoundary;
        mDumpStream = dumpStream;
    }

    public String getMimeBoundary() {
        return mMimeBoundary;
    }

    public InputStream getStream() {
        return mDumpStream;
    }

    public void close() {
        mMinidumpFile.delete();
    }

    /**
     * Handles creation of stream for uploading of crash dump
     * along with appending other fields
     *
     */
    public static final class DumpStreamBuilder {
        private static final String TAG = "cr_DumpStreamBuilder";

        public static DumpStream constructDumpStream(
                String uuid, String applicationFeedback, String logcat, File minidumpFile) {
            ArrayList<InputStream> streamList = new ArrayList<>();

            String dumpFirstLine = extractDumpFirstLine(minidumpFile);
            String mimeBoundary = dumpFirstLine.substring(2);

            try {
                AddIfNotNull(streamList, getAppFeedbackStream(dumpFirstLine, applicationFeedback));
                AddIfNotNull(streamList, getUuidStream(dumpFirstLine, uuid));
                AddIfNotNull(streamList, getLogcatStream(dumpFirstLine, logcat));
                AddIfNotNull(streamList, getMinidumpStream(minidumpFile));

            } catch (UnsupportedCharsetException e) {
                // Should never happen; all Java implementations are required to support UTF-8.
                Log.wtf(TAG, "UTF-8 not supported", e);
            }
            return new DumpStream(minidumpFile, mimeBoundary,
                    new SequenceInputStream(Collections.enumeration(streamList)));
        }

        @VisibleForTesting
        protected static InputStream getUuidStream(String dumpFirstLine, String uuid) {
            if (!uuid.equals("")) {
                Log.i(TAG, "Including UUID: " + uuid);

                StringBuilder uuidBuilder = new StringBuilder();
                uuidBuilder.append(dumpFirstLine);
                uuidBuilder.append("\n");
                uuidBuilder.append(generateTextFieldMimeHeader("comments"));
                uuidBuilder.append(uuid);
                uuidBuilder.append("\n");

                return new ByteArrayInputStream(
                        uuidBuilder.toString().getBytes(Charset.forName("UTF-8")));
            } else {
                Log.d(TAG, "No UUID");
                return null;
            }
        }

        @VisibleForTesting
        protected static InputStream getAppFeedbackStream(
                String dumpFirstLine, String appFeedback) {
            if (!appFeedback.equals("")) {
                Log.i(TAG, "Including feedback");

                StringBuilder feedbackHeader = new StringBuilder();
                feedbackHeader.append(dumpFirstLine);
                feedbackHeader.append("\n");
                feedbackHeader.append(
                        generateFileMimeHeader("application_feedback.txt", "application.txt"));
                feedbackHeader.append(appFeedback);
                feedbackHeader.append("\n");

                return new ByteArrayInputStream(
                        feedbackHeader.toString().getBytes(Charset.forName("UTF-8")));
            } else {
                Log.d(TAG, "No Feedback");
                return null;
            }
        }
        @VisibleForTesting
        protected static InputStream getLogcatStream(String dumpFirstLine, String logcat) {
            if (!logcat.equals("")) {
                Log.i(TAG, "Including log file");

                StringBuilder logHeader = new StringBuilder();
                logHeader.append(dumpFirstLine);
                logHeader.append("\n");
                logHeader.append(generateFileMimeHeader("log.txt", "log.txt"));
                logHeader.append(logcat);
                logHeader.append("\n");

                return new ByteArrayInputStream(
                        logHeader.toString().getBytes(Charset.forName("UTF-8")));
            } else {
                Log.d(TAG, "No Logcat");
                return null;
            }
        }

        @VisibleForTesting
        protected static InputStream getMinidumpStream(File minidumpFile) {
            try {
                return new FileInputStream(minidumpFile);
            } catch (FileNotFoundException e) {
                Log.e(TAG, "Minidump not found", e);
                return null;
            }
        }

        @VisibleForTesting
        protected static void AddIfNotNull(
                ArrayList<InputStream> streamList, InputStream inputStream) {
            if (inputStream != null) {
                streamList.add(inputStream);
            }
        }

        // Dump file is already in multipart MIME format and has a boundary throughout.
        // Scrape the first line, remove two dashes, call that the "boundary" and add it
        // to the content-type.
        @VisibleForTesting
        protected static String extractDumpFirstLine(File minidumpFile) {
            String dumpFirstLine = "";
            try (FileInputStream dumpFileStream = new FileInputStream(minidumpFile)) {
                dumpFirstLine = DumpStreamUtils.getFirstLineFromStream(dumpFileStream);
            } catch (IOException e) {
                Log.e(TAG, "Error occurred trying to parse crash dump mimeBoundary", e);
            }
            return dumpFirstLine;
        }

        // header for MIME multipart text field
        @VisibleForTesting
        protected static String generateTextFieldMimeHeader(String name) {
            return "Content-Disposition: form-data; name=\"" + name + "\"\n"
                    + "Content-Type: text/plain\n\n";
        }

        // header for MIME multipart file field
        @VisibleForTesting
        protected static String generateFileMimeHeader(String name, String filename) {
            return "Content-Disposition: form-data; name=\"" + name + "\"; filename=\"" + filename
                    + "\"\nContent-Type: text/plain\n\n";
        }
    }
}
