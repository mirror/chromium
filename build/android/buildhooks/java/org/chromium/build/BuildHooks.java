// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.build;

/**
 * This class is inserted in build, all Java targets have dependence on it. Used for hooks needed
 * when bytecode rewriting. Static convenience methods are used to minimize the amount of code
 * required to be manually generated when bytecode rewriting.
 *
 * This class contains default implementations for all methods - these are used when no other
 * implementation is supplied to an android_apk target (via build_hooks_impl_deps).
 */
public abstract class BuildHooks {
    private static BuildHooksImpl sInstance;

    private static BuildHooks get() {
        if (sInstance == null) {
            sInstance = new BuildHooksImpl();
        }
        return sInstance;
    }

    private Callback<AssertionError> mAssertCallback;

    /**
     * Handle AssertionError, decide whether throw or report without crashing base on gn arg.
     * This method is inserted to handle any assert failure by java_assertion_enabler.
     */
    protected void assertFailureHandlerImpl(AssertionError assertionError) {
        if (mAssertCallback != null) {
            mAssertCallback.run(assertionError);
        } else {
            throw assertionError;
        }
    }
    public static void assertFailureHandler(AssertionError assertionError) {
        get().assertFailureHandlerImpl(assertionError);
    }

    /**
     * Set the callback function that handles assert failure.
     * This should be called from attachBaseContext.
     */
    protected void setAssertCallbackImpl(Callback<AssertionError> callback) {
        mAssertCallback = callback;
    }
    public static void setAssertCallback(Callback<AssertionError> callback) {
        get().setAssertCallbackImpl(callback);
    }
}
