// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.base.annotations.SuppressFBWarnings;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.PrintStream;

/**
 * TestTraceEvent is a modified version of TraceEvent, intended for
 * tracing test runs.
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

    /**
     * Enable tracing, and set a specific output file.
     *
     * @param file Which file to append the trace data to.
     */
    public static synchronized void enable(File outputFile) {
        if (!sEnabled) {
            sEnabled = true;

            setOutputFile(outputFile);
            sTraceStrings = new JSONArray();
        }
    }

    /**
     * Disabling of tracing will dump trace data to the system log.
     */
    public static synchronized void disable() {
        if (sEnabled) {
            dumpTraceOutput();
            sTraceStrings = null;

            sEnabled = false;
        }
    }

    /**
     * @return True if tracing is enabled, false otherwise.
     */
    public static synchronized boolean isEnabled() {
        return sEnabled;
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
        long timeMicroseconds = System.currentTimeMillis() * 1000;

        try {
            JSONObject traceObj = new JSONObject();
            traceObj.put("cat", "Java");
            traceObj.put("ts", timeMicroseconds);
            traceObj.put("ph", type);
            traceObj.put("name", name);
            traceObj.put("id", id);

            sTraceStrings.put(traceObj);
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Sets a file to dump the results to, when {@link #disable()} is called.
     *
     * @param file Which file to append the trace data to.
     */
    private static synchronized void setOutputFile(File file) {
        sOutputFile = file;
    }

    /**
     * Dump all tracing data we have saved up to the log.
     * Output as JSON for parsing convenience.
     */
    private static void dumpTraceOutput() {
        if (sEnabled) {
            String json = sTraceStrings.toString();

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
