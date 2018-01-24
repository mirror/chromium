// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import org.chromium.base.VisibleForTesting;

import java.io.IOException;

/**
 * Extracts logcat out of Android devices and elide PII sensitive info from it.
 *
 * <p>Elided information includes: Emails, IP address, MAC address, URL/domains as well as
 * Javascript console messages.
 */
abstract class ElidedLogcatProvider {
    protected abstract void getRawLogcat(RawLogcatCallback rawLogcatCallback)
            throws IOException, InterruptedException;

    protected interface RawLogcatCallback { public void onLogsDone(Iterable<String> logs); }
    public interface LogcatCallback { public void onLogsDone(String logs); }

    public void getElidedLogcat(LogcatCallback callback) throws IOException, InterruptedException {
        getRawLogcat((Iterable<String> rawLogs) -> { callback.onLogsDone(elideLogcat(rawLogs)); });
    }

    @VisibleForTesting
    private static String elideLogcat(Iterable<String> rawLogcat) {
        if (rawLogcat == null) return "";

        StringBuilder builder = new StringBuilder();
        for (String line : rawLogcat) {
            builder.append(LogcatElision.elide(line));
            builder.append("\n");
        }
        return builder.toString();
    }
}
