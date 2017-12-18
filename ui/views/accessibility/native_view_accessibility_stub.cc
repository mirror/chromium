// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/view_accessibility.h"

namespace views {

// static
std::unique_ptr<ViewAccessibility> ViewAccessibility::Create(View* view) {
  return std::make_unique<ViewAccessibility>(view);
}

}  // namespace views
