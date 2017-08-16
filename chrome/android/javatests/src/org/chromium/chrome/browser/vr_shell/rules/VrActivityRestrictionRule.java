// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.rules;

import org.junit.Assert;
import org.junit.Assume;
import org.junit.rules.RuleChain;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.base.test.params.ParameterSet;

import java.util.ArrayList;
import java.util.concurrent.Callable;

/**
 * Rule that conditionally skips a test if the current VrTestRule's Activity is not
 * one of the supported Activity types for the test.
 */
public class VrActivityRestrictionRule implements TestRule {
    private String mCurrentRestrictionString;

    public VrActivityRestrictionRule(String currentRestrictionString) {
        mCurrentRestrictionString = currentRestrictionString;
    }

    @Override
    public Statement apply(final Statement base, final Description desc) {
        // Check if the test has a VrActivityRestriction annotation
        VrActivityRestriction annotation = desc.getAnnotation(VrActivityRestriction.class);
        if (annotation == null) {
            if (mCurrentRestrictionString.equals(VrActivityRestriction.CTA)) {
                // Default to running in ChromeTabbedActivity if no restriction annotation
                return base;
            }
            return generateIgnoreStatement();
        }

        String[] activities = annotation.value();
        for (int i = 0; i < activities.length; i++) {
            if (activities[i].equals(mCurrentRestrictionString)
                    || activities[i].equals(VrActivityRestriction.ALL)) {
                return base;
            }
        }
        return generateIgnoreStatement();
    }

    private Statement generateIgnoreStatement() {
        return new Statement() {
            @Override
            public void evaluate() {
                Assume.assumeTrue("Test ignored due to " + mCurrentRestrictionString
                                + " not being specified for the test",
                        false);
            }
        };
    }

    /**
     * Creates the list of VrTestRules that are currently supported for use in test
     * parameterization.
     */
    public static ArrayList<ParameterSet> generateDefaultVrTestRuleParameters() {
        ArrayList<ParameterSet> parameters = new ArrayList<ParameterSet>();
        parameters.add(new ParameterSet()
                               .value(new Callable<ChromeTabbedActivityVrTestRule>() {
                                   @Override
                                   public ChromeTabbedActivityVrTestRule call() {
                                       return new ChromeTabbedActivityVrTestRule();
                                   }
                               })
                               .name("ChromeTabbedActivity"));

        parameters.add(new ParameterSet()
                               .value(new Callable<CustomTabActivityVrTestRule>() {
                                   @Override
                                   public CustomTabActivityVrTestRule call() {
                                       return new CustomTabActivityVrTestRule();
                                   }
                               })
                               .name("CustomTabActivity"));

        parameters.add(new ParameterSet()
                               .value(new Callable<WebappActivityVrTestRule>() {
                                   @Override
                                   public WebappActivityVrTestRule call() {
                                       return new WebappActivityVrTestRule();
                                   }
                               })
                               .name("WebappActivity"));

        return parameters;
    }

    /**
     * Creates a RuleChain that applies the VrActivityRestrictionRule before the given VrTestRule.
     */
    public static RuleChain wrapRuleInVrActivityRestrictionRule(TestRule rule) {
        Assert.assertTrue("Given rule is a VrTestRule", rule instanceof VrTestRule);
        return RuleChain
                .outerRule(
                        new VrActivityRestrictionRule(((VrTestRule) rule).getRestrictionString()))
                .around(rule);
    }
}
