// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package util;

import org.chromium.base.test.TestTraceEvent;

/**
 * Provides helper methods for tracing Java functions.
 */
public class Tracing {
    public static void traceBegin(String methodName) {
        System.out.println("BEGINNING " + methodName);
        TestTraceEvent.begin(methodName);
    }

    public static void traceEnd(String methodName) {
        System.out.println("ENDING " + methodName);
        TestTraceEvent.end(methodName);
    }
}
