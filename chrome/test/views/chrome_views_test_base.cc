// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/views/chrome_views_test_base.h"

void ChromeViewsTestBase::SetUp() {
  views::ViewsTestBase::SetUp();
  provider_ = base::MakeUnique<ChromeLayoutProvider>();
}
