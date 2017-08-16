// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.rules;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * An annotation for listing what types of ChromeActivity the test should be restricted to.
 * This is meant to only be used with test classes that have a VrActivityRestrictionRule,
 * otherwise the annotation will have no effect.
 *
 * For example, the following would restrict a test to only run in ChromeTabbedActivity and
 * CustomTabActivity:
 *     <code>
 *     @VrActivityRestriction({VrActivityRestriction.CTA, VrActivityRestriction.CCT})
 *     </code>
 * If a test is not annotated with this and VrActivityRestrictionRule is present, the test
 * will default to only running in ChromeTabbedActivity (regular Chrome).
 */
@Target({ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
public @interface VrActivityRestriction {
    /** Specifies that the test is valid in ChromeTabbedActivity. */
    public static final String CTA = "Run_in_ChromeTabbedActivity";

    /** Specifies that the test is valid in CustomTabActivity. */
    public static final String CCT = "Run_in_CustomTabActivity";

    /** Specifies that the test is valid in WebappActivity. */
    public static final String WAA = "Run_in_WebappActivity";

    /**
     * Specifies that the test is valid in all supported activities. Equivalent to explicitly
     * using all of the above restrictions.
     */
    public static final String ALL = "Run_in_AllActivities";

    /**
     * @return A list of activity restrictions.
     */
    public String[] value();
}
