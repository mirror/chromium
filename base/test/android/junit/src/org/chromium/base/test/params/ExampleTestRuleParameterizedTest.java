// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.params;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.model.Statement;

import org.chromium.base.test.params.ParameterAnnotations.ClassParameter;
import org.chromium.base.test.params.ParameterAnnotations.UseRunnerDelegate;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;

/**
 * Example test that uses ParameterRunner
 */
@RunWith(ParameterizedRunner.class)
@UseRunnerDelegate(BlockJUnit4RunnerDelegate.class)
public class ExampleTestRuleParameterizedTest {
    @ClassParameter
    private static List<ParameterSet> sClassParams = new ArrayList<>();

    /**
     * This part can be wrapped around a static function.
     *
     * E.g.
     * <code>
     * @ClassParameter private static List<ParameterSet> sClassParams = getDefaultRuleParameters();
     * </code>
     */
    static {
        sClassParams.add(new ParameterSet().value(createTestRuleCallable("hello")).name("hello"));
        sClassParams.add(new ParameterSet().value(createTestRuleCallable("world")).name("world"));
        sClassParams.add(new ParameterSet().value(createTestRuleCallable("xx")).name("xx"));
        sClassParams.add(new ParameterSet().value(createTestRuleCallable("yy")).name("yy"));
        sClassParams.add(new ParameterSet().value(createTestRuleCallable("zz")).name("zz"));
    }

    @Rule
    public ExampleTestRule mRule;

    @Rule
    public TestName mTestName = new TestName();

    public ExampleTestRuleParameterizedTest(Callable<ExampleTestRule> callable) throws Exception {
        mRule = callable.call();
    }

    /**
     * Test to make sure the current ExampleTestRule is a distinctivly parameterized one by
     * check if current test name contains the name string stored in the current rule.
     */
    @Test
    public void testSimple() {
        Assert.assertTrue(mTestName.getMethodName().contains(mRule.getName()));
    }

    private static Callable<ExampleTestRule> createTestRuleCallable(final String name) {
        return new Callable<ExampleTestRule>() {
            @Override
            public ExampleTestRule call() {
                return new ExampleTestRule(name);
            }
        };
    }
    static class ExampleTestRule implements TestRule {
        private final String mName;

        ExampleTestRule(String name) {
            mName = name;
        }

        @Override
        public Statement apply(final Statement base, final Description desc) {
            return base;
        }

        public String getName() {
            return mName;
        }
    }

}

