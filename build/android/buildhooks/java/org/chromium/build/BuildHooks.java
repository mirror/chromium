// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.build;

/**
 * All Java targets that support android have dependence on this class.
 */
public abstract class BuildHooks {
    private static Callback<AssertionError> sReportAssertionCallback;

    /**
     * This method is inserted to handle any assert failure by java_assertion_enabler.
     */
    public static void assertFailureHandler(AssertionError assertionError) {
        if (BuildHooksConfig.REPORT_JAVA_ASSERT) {
            if (sReportAssertionCallback != null) {
                sReportAssertionCallback.run(assertionError);
            }
        } else {
            throw assertionError;
        }
    }

    /**
     * Set the callback function that handles assert failure.
     * This should be called from attachBaseContext.
     */
    public static void setReportAssertionCallback(Callback<AssertionError> callback) {
        sReportAssertionCallback = callback;
    }
}
