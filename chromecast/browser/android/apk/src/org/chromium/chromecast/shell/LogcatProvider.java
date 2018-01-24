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
abstract class LogcatProvider {
    public abstract void getLogcat(LogcatCallback callback)
            throws IOException, InterruptedException;
    public interface LogcatCallback { public void onLogsDone(String logs); }

    @VisibleForTesting
    static String elideLogcat(Iterable<String> rawLogcat) {
        StringBuilder builder = new StringBuilder();
        for (String line : rawLogcat) {
            builder.append(LogcatElision.elide(line));
            builder.append("\n");
        }
        return builder.toString();
    }
}
