// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net.urlconnection;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.net.CronetTestRule;

/**
 * Test for CronetURLStreamHandlerFactory.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@SuppressWarnings("deprecation")
public class CronetURLStreamHandlerFactoryTest {
    @Rule
    public CronetTestRule mTestRule = new CronetTestRule();

    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testRequireConfig() throws Exception {
        mTestRule.startCronetTestFramework();
        try {
            new CronetURLStreamHandlerFactory(null);
        } catch (NullPointerException e) {
            Assert.assertEquals("CronetEngine is null.", e.getMessage());
        }
    }
}
