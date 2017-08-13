// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.rules;

/**
 * Interface to be implemented by *VrTestRule rules, which allows them to be
 * conditionally skipped when used in conjunction with VrActivityRestrictionRule.
 */
public interface VrTestRule {
    /**
     * Get the rule's restriction string, typically identical to those in VrActivityRestriction.
     */
    public String getRestrictionString();
}
