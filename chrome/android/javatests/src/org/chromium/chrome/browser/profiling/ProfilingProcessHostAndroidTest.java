// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.profiling;

import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import android.support.test.filters.MediumTest;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.browser.ChromeTabbedActivity;
// import org.chromium.net.test.EmbeddedTestServer;
//
// import java.io.File;
//

/**
 * Test suite for out of process heap profiling.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG})
public class ProfilingProcessHostAndroidTest {
    @Rule
	public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();
    // public ChromeTabbedActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
    //         new ChromeTabbedActivityTestRule<>(ChromeTabbedActivity.class);
    // public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
    //               new ChromeActivityTestRule<>(ChromeActivity.class);

		@Before
    public void setUp() throws InterruptedException {
				android.util.Log.w("string_test1", "message1");
        mActivityTestRule.startMainActivityOnBlankPage();
				android.util.Log.w("string_test2", "message2");
    }


    @Test
    @MediumTest
    public void testTraceFileCreation() throws Exception {
				android.util.Log.w("string_test3", "message3");

//				mActivityTestRule.startMainActivityOnBlankPage();
//        final ProfilingProcessHostAndroid profilingProcessHost = new ProfilingProcessHostAndroid();
//				profilingProcessHost.startProfilingBrowserProcess();
//				profilingProcessHost.takeTrace();
//				Thread.sleep(3000);
//				Assert.assertTrue(false);


        // EmbeddedTestServer testServer =
        //         EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        // try {
        //     // Launch chrome
        //     mActivityTestRule.startMainActivityWithURL(
        //             testServer.getURL("/chrome/test/data/android/simple.html"));
        //     String expectedTitle = "Activity test page";
        //     TabModel model = mActivityTestRule.getActivity().getCurrentTabModel();
        //     String title = model.getTabAt(model.index()).getTitle();
        //     Assert.assertEquals(expectedTitle, title);
        // } finally {
        //     testServer.stopAndDestroyServer();
        // }
      // Do nothing.
    }
}
