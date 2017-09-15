// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.net.CronetTestRule.CronetTestFramework;
import org.chromium.net.CronetTestRule.OnlyRunNativeCronet;
import org.chromium.net.CronetTestRule.RequiresMinApi;
import org.chromium.net.impl.CronetUrlRequestContext;
import org.chromium.net.impl.JavaCronetEngine;

/**
 * Tests features of CronetTestBase.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class CronetTestBaseTest {
    @Rule
    public CronetTestRule mTestRule = new CronetTestRule();

    @Rule
    public TestName mTestName = new TestName();

    private CronetTestFramework mTestFramework;
    /**
     * For any test whose name contains "MustRun", it's enforced that the test must run and set
     * {@code mTestWasRun} to {@code true}.
     */
    private boolean mTestWasRun;
    /**
     * For {@link #testRunBothImplsMustRun}, use {@link #mTestNativeImplRun} to verify that the
     * test is run against the native implementation.
     */
    private boolean mTestNativeImplRun;

    @Before
    public void setUp() throws Exception {
        mTestFramework = mTestRule.startCronetTestFramework();
    }

    @After
    public void tearDown() throws Exception {
        if (mTestName.getMethodName().contains("MustRun") && !mTestWasRun) {
            Assert.fail(mTestName.getMethodName() + " should have run but didn't.");
        }
    }

    @Test
    @SmallTest
    @RequiresMinApi(999999999)
    @Feature({"Cronet"})
    public void testRequiresMinApiDisable() {
        Assert.fail("RequiresMinApi failed to disable.");
    }

    @Test
    @SmallTest
    @RequiresMinApi(-999999999)
    @Feature({"Cronet"})
    public void testRequiresMinApiMustRun() {
        mTestWasRun = true;
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testRunBothImplsMustRun() {
        if (mTestRule.testingJavaImpl()) {
            Assert.assertFalse(mTestWasRun);
            Assert.assertTrue(mTestNativeImplRun);
            mTestWasRun = true;
            Assert.assertEquals(mTestFramework.mCronetEngine.getClass(), JavaCronetEngine.class);
        } else {
            Assert.assertFalse(mTestWasRun);
            Assert.assertFalse(mTestNativeImplRun);
            mTestNativeImplRun = true;
            Assert.assertEquals(
                    mTestFramework.mCronetEngine.getClass(), CronetUrlRequestContext.class);
        }
    }

    @Test
    @SmallTest
    @Feature({"Cronet"})
    @OnlyRunNativeCronet
    public void testRunOnlyNativeMustRun() {
        Assert.assertFalse(mTestRule.testingJavaImpl());
        Assert.assertFalse(mTestWasRun);
        mTestWasRun = true;
        Assert.assertEquals(mTestFramework.mCronetEngine.getClass(), CronetUrlRequestContext.class);
    }
}
