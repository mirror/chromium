// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.annotations.SuppressFBWarnings;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.PrintStream;

/**
 * TestTraceEvent is a modified version of TraceEvent, intended for
 * tracing test runs. It is based on the PerfTraceEvent class, which
 * had similar functionality but was unused.
 */
@SuppressFBWarnings("CHROMIUM_SYNCHRONIZED_METHOD")
public class TestTraceEvent {
    private static final String TAG = "TestTraceEvent";

    /** The event types understood by the trace scripts. */
    private enum EventType {
        START("S"),
        FINISH("F"),
        INSTANT("I");

        private final String mTypeStr;

        EventType(String typeStr) {
            mTypeStr = typeStr;
        }

        @Override
        public String toString() {
            return mTypeStr;
        }
    }

    private static File sOutputFile;

    private static boolean sEnabled;

    // A list of trace event strings.
    private static JSONArray sTraceStrings;

    // Nanosecond start time of tracing.
    private static long sBeginNanoTime;

    /**
     * Enable tracing.
     * Disabling of tracing will dump trace data to the system log.
     */
    @VisibleForTesting
    public static synchronized void setEnabled(boolean enabled) {
        if (sEnabled == enabled) {
            return;
        }
        if (enabled) {
            sBeginNanoTime = System.nanoTime();
            sTraceStrings = new JSONArray();
        } else {
            dumpTraceOutput();
            sTraceStrings = null;
        }
        sEnabled = enabled;
    }

    /**
     * @return True if tracing is enabled, false otherwise.
     */
    @VisibleForTesting
    public static synchronized boolean enabled() {
        return sEnabled;
    }

    /**
     * Set the current time, by recording the delta from System.nanoTime().
     */
    public static synchronized void setCurrentNanoTime(long newTime) {
        // Now current time - newTime will be treated as 0, so current time will
        // be treated as newTime.
        sBeginNanoTime = System.nanoTime() - newTime;
    }

    /**
     * Record an "instant" trace event.  E.g. "screen update happened".
     */
    public static synchronized void instant(String name) {
        if (sEnabled) {
            saveTraceString(name, name.hashCode(), EventType.INSTANT);
        }
    }

    /**
     * Record an "begin" trace event.
     * Begin trace events should have a matching end event.
     */
    @VisibleForTesting
    public static synchronized void begin(String name) {
        if (sEnabled) {
            saveTraceString(name, name.hashCode(), EventType.START);
        }
    }

    /**
     * Record an "end" trace event, to match a begin event.  The time
     * delta between begin and end is usually interesting to graph
     * code.
     */
    @VisibleForTesting
    public static synchronized void end(String name) {
        if (sEnabled) {
            saveTraceString(name, name.hashCode(), EventType.FINISH);
        }
    }

    /**
     * Save a trace event as a JSON dict.  The format mirrors a TraceEvent dict.
     *
     * @param name The trace data
     * @param id The id of the event
     * @param type the type of trace event (I, S, F)
     */
    private static void saveTraceString(String name, long id, EventType type) {
        long timestampUs = (System.nanoTime() - sBeginNanoTime) / 1000;

        try {
            JSONObject traceObj = new JSONObject();
            traceObj.put("cat", "Java");
            traceObj.put("ts", timestampUs);
            traceObj.put("ph", type);
            traceObj.put("name", name);
            traceObj.put("id", id);

            sTraceStrings.put(traceObj);
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Sets a file to dump the results to.  If {@code file} is {@code null}, it will be dumped
     * to STDOUT, otherwise the JSON data will be appended to {@code file}.  This should be
     * called before enabling tracing.  When {@link #setEnabled(boolean)} is called with
     * {@code false}, the data will be dumped.
     *
     * @param file Which file to append the trace data to.  If {@code null}, the trace data
     *             will be sent to STDOUT.
     */
    @VisibleForTesting
    public static synchronized void setOutputFile(File file) {
        sOutputFile = file;
    }

    /**
     * Dump all tracing data we have saved up to the log.
     * Output as JSON for parsing convenience.
     */
    private static void dumpTraceOutput() {
        String json = sTraceStrings.toString();
        if (sOutputFile == null) {
            System.out.println(json);
        } else {
            try {
                PrintStream stream = new PrintStream(new FileOutputStream(sOutputFile, true));
                try {
                    stream.print(json);
                } finally {
                    try {
                        stream.close();
                    } catch (Exception ex) {
                        Log.e(TAG, "Unable to close trace output file.");
                    }
                }
            } catch (FileNotFoundException ex) {
                Log.e(TAG, "Unable to dump trace data to output file.");
            }
        }
    }
}
