// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// WARNING: When modifying these tests, ensure that corresponding internal tests
// in .../memlog/* are updated.
//
// This class contains end-to-end client tests for heap profiling, from
// profiling start [dynamic or by command line flag], all the way through
// verifying the contents of the trace. The traces are uploaded and processed on
// the server. Test failures due to changes in the trace format will also cause
// problems with the server tests and functionality.

package org.chromium.chrome.browser.profiling_host;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Test suite for out of process heap profiling.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class ProfilingProcessHostAndroidTest {
    private static final String TAG = "ProfilingProcessHostAndroidTest";
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"memlog=browser", "memlog-stack-mode=native-include-thread-names"})
    public void testModeBrowser() throws Exception {
        TestAndroidShim profilingProcessHost = new TestAndroidShim();
        Assert.assertTrue(profilingProcessHost.runTestForMode(
                "browser", false, "native-include-thread-names"));
    }

    @Test
    @MediumTest
    public void testModeBrowserDynamic() throws Exception {
        TestAndroidShim profilingProcessHost = new TestAndroidShim();
        Assert.assertTrue(profilingProcessHost.runTestForMode("browser", true, "native"));
    }

    @Test
    @MediumTest
    public void testModeBrowserDynamicPseudo() throws Exception {
        TestAndroidShim profilingProcessHost = new TestAndroidShim();
        Assert.assertTrue(profilingProcessHost.runTestForMode("browser", true, "pseudo"));
    }

    // Non-browser processes must be profiled with a command line flag, since
    // otherwise, profiling will start after the relevant processes have been
    // created, thus that process will be not be profiled.
    @Test
    @MediumTest
    @CommandLineFlags.Add({"memlog=all-renderers", "memlog-stack-mode=pseudo"})
    // Disabled: https://crbug.com/804412
    @DisabledTest
    public void testModeRendererPseudo() throws Exception {
        TestAndroidShim profilingProcessHost = new TestAndroidShim();
        Assert.assertTrue(profilingProcessHost.runTestForMode("all-renderers", false, "pseudo"));
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"memlog=gpu", "memlog-stack-mode=pseudo"})
    public void testModeGpuPseudo() throws Exception {
        TestAndroidShim profilingProcessHost = new TestAndroidShim();
        Assert.assertTrue(profilingProcessHost.runTestForMode("gpu", false, "native"));
    }
}
