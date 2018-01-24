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
 * Utility class for dump streams
 *
 */
public final class DumpStreamUtils {
    private static final String TAG = "cr_DumpStreamUtils";

    /**
     * Gets the first line from an input stream
     * Mutates given input stream.
     *
     * @return First line of the input stream.
     * @throws IOException
     */
    protected static String getFirstLineFromStream(InputStream inputStream) throws IOException {
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
