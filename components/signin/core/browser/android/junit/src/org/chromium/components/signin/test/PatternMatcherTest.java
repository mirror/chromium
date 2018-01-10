// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.signin.test;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.components.signin.util.PatternMatcher;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Test class for {@link PatternMatcher}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class PatternMatcherTest {
    @Test
    @SmallTest
    public void testPatternMatcher() {
        Assert.assertTrue(matchPattern("", ""));
        Assert.assertFalse(matchPattern("test@gmail.com", ""));

        Assert.assertTrue(matchPattern("test@gmail.com", "test@gmail.com"));
        Assert.assertFalse(matchPattern("test2@gmail.com", "test@gmail.com"));
        Assert.assertFalse(matchPattern("thetest@gmail.com", "test@gmail.com"));
        Assert.assertFalse(matchPattern("test@gmail.com.example.com", "test@gmail.com"));

        Assert.assertTrue(matchPattern("test@gmail.com", "*"));
        Assert.assertTrue(matchPattern("test@gmail.com", "*.com"));
        Assert.assertFalse(matchPattern("test@gmail.org", "*.com"));
        Assert.assertTrue(matchPattern("test@gmail.com", "*gmail.com"));
        Assert.assertTrue(matchPattern("test@thegmail.com", "*gmail.com"));
        Assert.assertFalse(matchPattern("test@gmail.com", "*@gmail.org"));
        Assert.assertFalse(matchPattern("gmail.com@example.com", "*gmail.com"));
        Assert.assertTrue(matchPattern("test@gmail.example.com", "test@*.example.com"));

        // Test escaping
        Assert.assertTrue(matchPattern("test*@gmail.com", "test\\*@gmail.com"));
        Assert.assertFalse(matchPattern("test@gmail.com", "test\\*@gmail.com"));

        Assert.assertFalse(matchPattern("test@gmail.com", "\\*"));
        Assert.assertTrue(matchPattern("\\test@gmail.com", "\\\\*"));
        // Escaping is allowed for all characters, not just asterisk
        Assert.assertTrue(matchPattern("test@gmail.com", "\\t\\e\\s\\t\\@\\gmail.com"));
    }

    @Test
    @SmallTest
    public void testMalformedEscapeSequence() {
        try {
            matchPattern("", "account@gmail.com\\");
            Assert.fail("Should've thrown exception!");
        } catch (IllegalArgumentException ignored) {
        }
    }

    private boolean matchPattern(String string, String pattern) {
        return new PatternMatcher(pattern).isMatch(string);
    }
}
