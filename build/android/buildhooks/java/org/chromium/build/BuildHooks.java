// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.build;

/**
 * All Java targets that support android have dependence on this class.
 */
public abstract class BuildHooks {
    private static Callback<AssertionError> sAssertCallback;

    /**
     * This method is inserted to handle any assert failure by java_assertion_enabler.
     */
    public static void assertFailureHandler(AssertionError assertionError) {
        if (sAssertCallback != null) {
            sAssertCallback.run(assertionError);
        } else {
            //  It is vital that when assertions are enabled on a Canary, that we set this handler
            //  first thing, and that we don't ship any other apks that don't set the callback in
            //  their application classes (e.g. webview)
            throw assertionError;
        }
    }

    /**
     * Set the callback function that handles assert failure.
     * This should be called from attachBaseContext.
     */
    public static void setAssertCallback(Callback<AssertionError> callback) {
        sAssertCallback = callback;
    }
}
