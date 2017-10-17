// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test;

import android.support.test.rule.ActivityTestRule;
import android.support.test.rule.UiThreadTestRule;

import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.base.test.util.SkipRule;

import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * TestRule that combine multiple test rules together and create Statement with
 * an ordered map that decides which type of test rules runs first/last.
 */
//TODO(yolandyan): provide a log statment to pretty print all the rules under CTR
public final class CompoundTestRule implements TestRule {

    // Default order map, the number represents the running order
    public static final Map<Class<? extends TestRule>, Integer> DEFAULT_ORDERMAP;

    static {
        Map<Class<? extends TestRule>, Integer> map = new HashMap<>();
        map.put(CommandLineTestRule.class, 0);
        map.put(SkipRule.class, 10);
        map.put(PreTestHooks.class, 20);
        map.put(ActivityTestRule.class, 30);
        map.put(UiThreadTestRule.class, 40);
        DEFAULT_ORDERMAP = Collections.unmodifiableMap(map);
    }

    private final Map<Class<? extends TestRule>, List<TestRule>> mTestRuleMap = new HashMap<>();

    private Map<Class<? extends TestRule>, Integer> mOrderMap = new HashMap<>(DEFAULT_ORDERMAP);

    private List<TestRule> mTestRules;

    @Override
    public Statement apply(Statement base, Description desc) {
        return wrapsAllStatement(base, desc);
    }


    /**
     * Add one TestRule to this CTR
     */
    public CompoundTestRule add(TestRule testRule){
        add(testRule.getClass(), testRule);
        addTestRuleToKnownSuperType(testRule);
        mTestRules.add(testRule);
        return this;
    }

    /**
     * Add multiple TestRule to this CTR
     */
    public CompoundTestRule addAll(TestRule testRule, TestRule...testRules){
        add(testRule);
        for (TestRule rule : testRules) {
            add(rule);
        }
        return this;
    }

    /**
     * Set the order map for this CTR.
     *
     * The order map specify the running order of the type of test rules
     */
    public CompoundTestRule setOrder(Map<Class<? extends TestRule>, Integer> map) {
        mOrderMap = map;
        return this;
    }

    /**
     * Get the instance of T type of TestRule.
     *
     * If there are more than one T type of TestRule, return the first instance
     */
    public <T extends TestRule> T getTestRule(Class<T> type) {
        if (!mTestRuleMap.containsKey(type)) {
            throw new IllegalArgumentException("TestRule class not found");
        }
        return type.cast(mTestRuleMap.get(type).get(0));
    }

    /**
     * Get all the instances of T type of TestRule.
     */
    @SuppressWarnings("unchecked")
    public <T extends TestRule> List<T> getAllTestRules(Class<T> type) {
        if (!mTestRuleMap.containsKey(type)) {
            throw new IllegalArgumentException("TestRule class not found");
        }
        return (List<T>) mTestRuleMap.get(type);
    }

    /**
     * Add TestRule runtime type to order map based on its super class that is already in mOrderMap.
     *
     * Because TestRules can have inheritance, the run time type of the TestRule can be
     * different from the one we specified in ordered map. For example, we specified
     * ActivityTestRule to have order 30, but the runtime type of most of our activity test rule
     * is ChromeActivityTestRule. This method adds ChromeActivityTestRule.class to order map
     * with the same order as ActivityTestRule
     */
    private void addTestRuleToKnownSuperType(TestRule testRule) {
        for (Class<? extends TestRule> type : mOrderMap.keySet()) {
            if (type.isInstance(testRule) && !mOrderMap.containsKey(testRule.getClass())) {
                mOrderMap.put(testRule.getClass(), mOrderMap.get(type));
                add(type, testRule);
            }
        }
    }

    private void add(Class<? extends TestRule> type, TestRule testRule) {
        if (mTestRuleMap.containsKey(type)) {
            mTestRuleMap.get(type).add(testRule);
        } else {
            List<TestRule> ruleList = new LinkedList<>();
            mTestRuleMap.put(type, ruleList);
        }
    }


    private Statement wrapsAllStatement(final Statement base, final Description desc) {
        Statement wrap = base;
        Collections.sort(mTestRules, Collections.reverseOrder(getComparator(mOrderMap)));
        for (TestRule testRule : mTestRules) {
            wrap = testRule.apply(wrap, desc);
        }
        return wrap;
    }

    private static Comparator<TestRule> getComparator(Map<Class<? extends TestRule>, Integer> map) {
        return new Comparator<TestRule>() {
            @Override
            public int compare(TestRule entry1, TestRule entry2) {
                Integer firstEntryOrder = map.containsKey(entry1.getClass()) ? map.get(
                        entry1.getClass()) : Integer.MAX_VALUE;
                Integer secondEntryOrder = map.containsKey(entry2.getClass()) ? map.get(
                        entry2.getClass()) : Integer.MAX_VALUE;
                return Integer.compare(firstEntryOrder, secondEntryOrder);
            }
        };
    }
}
