// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

/**
 *
 * See {@link android.webkit.ConsoleMessage}. */
public class AwConsoleMessage {
    // This must be kept in sync with the WebCore enum in WebCore/page/Console.h
    public enum MessageLevel { TIP, LOG, WARNING, ERROR, DEBUG }

    private MessageLevel mLevel;
    private String mMessage;
    private String mSourceId;
    private int mLineNumber;

    public AwConsoleMessage(
            String message, String sourceId, int lineNumber, MessageLevel msgLevel) {
        mMessage = message;
        mSourceId = sourceId;
        mLineNumber = lineNumber;
        mLevel = msgLevel;
    }

    public MessageLevel messageLevel() {
        return mLevel;
    }

    public String message() {
        return mMessage;
    }

    public String sourceId() {
        return mSourceId;
    }

    public int lineNumber() {
        return mLineNumber;
    }
}
