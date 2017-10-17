// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import org.chromium.base.test.CompoundTestRule;

public class ChromeTestRules {
    public static CompoundTestRule getDefaultCompoundTestRule() {
        return new CompoundTestRule().addAll(
                new MinSdkTestRule(),
                new CommandLineTestRule(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
                    ChromeActivityTestRule.DISABLE_NETWORK_PREDICTION_FLAG),
                new ChromeRestrictionRule(),
                new UiRestrictionRule());
    }

    public static CompoundTestRule getTabbedActivityCTR() {
        return getDefaultCompoundTestRule().add(new ChromeTabbedActivityTestRule);
    }
}
