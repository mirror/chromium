// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/accessibility/testing/AccessibilityTest.h"

namespace blink {
  
  //
  // Get selection tests.
  // Retrieving a selection with endpoints which have no corresponding objects in the accessibility tree, e.g. which are aria-hidden, should shring |AXRange| to valid endpoints.
  //
  
  //
  // Set selection tests.
  // Setting the selection from an |AXRange| that has endpoints which are not present in the layout tree should shring the selection to visible endpoints.
  //
  
}  // namespace blink
