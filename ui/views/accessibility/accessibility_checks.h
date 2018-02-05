// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ACCESSIBILITY_ACCESSIBILITY_CHECKS_H_
#define UI_VIEWS_ACCESSIBILITY_ACCESSIBILITY_CHECKS_H_

// Runs UI accessibility checks on active browsers' View hierarchy.
// If there are failures returns false and sets |error_message|.
// bool RunUIAccessibilityChecks(std::string* error_message);

bool CheckViewAccessibility(views::View* view,
                            ui::AXNodeData& data,
                            std::string* error_message);

#endif  // UI_VIEWS_ACCESSIBILITY_ACCESSIBILITY_CHECKS_H_
