// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.devtools;

/**
 * Interface to send devtools commands to the native DevToolsAgentHost for a given Tab.
 *
 * See Tab.getOrCreateDevToolsAgentHost().
 */
public interface DevToolsAgentHost {
    /** Used to receive the results of a devtools command asynchronously. */
    public interface ResultCallback {
        /** Exactly one of "result" or "error" will be null. */
        public void onDevToolsCommandResult(String result, String error);
    }

    /** Used to receive events from the devtools asynchronously. */
    public interface EventListener { public void onDevToolsEvent(String method, String params); }

    /**
     * Sends a devtools command to the underlying Tab.
     *
     * @param method The devtools command e.g. "Page.navigate"
     * @param params An optional JSON object with parameters to the method. May be null.
     * @param callback Will be invoked with the result of the command or an error.
     */
    public void sendCommand(String method, String params, ResultCallback callback);

    /**
     * Will be notified of any devtools events received on this page.
     *
     * For example, sending the "Page.enable" command will start triggering Page
     * navigation events asynchronously over this interface.
     */
    public void addEventListener(EventListener listener);

    public void removeEventListener(EventListener listener);

    /**
     * Shuts down the devtools connection.
     *
     * No more events will be received, and any subsequent sendCommand() calls
     * will fail.
     */
    public void close();
}
