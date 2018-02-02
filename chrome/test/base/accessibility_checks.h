// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_ACCESSIBILITY_CHECKS_H_
#define CHROME_TEST_BASE_ACCESSIBILITY_CHECKS_H_

// Runs UI accessibility checks on active browsers' View hierarchy.
// If there are failures returns false and sets |error_message|.
bool RunUIAccessibilityChecks(std::string* error_message);

#endif  // CHROME_TEST_BASE_ACCESSIBILITY_CHECKS_H_
