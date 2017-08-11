// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.test;

import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;

import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.base.PathUtils;
import org.chromium.base.ResourceExtractor;
import org.chromium.chrome.test.util.ApplicationData;
import org.chromium.chrome.test.util.browser.signin.SigninTestUtil;
import org.chromium.content.browser.test.NativeLibraryTestRule;

/**
 * JUnit test rule that takes care of important initialization for Chrome-specific tests, such as
 * initializing the AccountManagerFacade.
 */
public class ChromeBrowserTestRule extends NativeLibraryTestRule {
    private static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "chrome";

    private void setUp(Instrumentation instrumentation) {
        ApplicationData.clearAppData(
                InstrumentationRegistry.getInstrumentation().getTargetContext());
        SigninTestUtil.setUpAuthForTest(instrumentation);

        // Extract Chrome's compressed locale paks.
        // TODO(zpeng): Maybe we should use ChromeBrowserInitializer for initialization.
        PathUtils.setPrivateDataDirectorySuffix(PRIVATE_DATA_DIRECTORY_SUFFIX);
        ResourceExtractor resourceExtractor = ResourceExtractor.get();
        resourceExtractor.startExtractingResources();
        resourceExtractor.waitForCompletion();

        loadNativeLibraryAndInitBrowserProcess();
    }

    @Override
    public Statement apply(final Statement base, Description description) {
        return super.apply(new Statement() {
            @Override
            public void evaluate() throws Throwable {
                /**
                 * Loads the native library on the activity UI thread (must not be called from the
                 * UI thread).  After loading the library, this will initialize the browser process
                 * if necessary.
                 */
                setUp(InstrumentationRegistry.getInstrumentation());
                try {
                    base.evaluate();
                } finally {
                    tearDown();
                }
            }
        }, description);
    }

    private void tearDown() {
        SigninTestUtil.resetSigninState();
        SigninTestUtil.tearDownAuthForTest();
    }
}
