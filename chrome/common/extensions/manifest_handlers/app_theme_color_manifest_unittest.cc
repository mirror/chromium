// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/common/extensions/manifest_tests/chrome_manifest_test.h"
#include "extensions/common/extension.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

class ThemeColorMatchesManifestTest : public ChromeManifestTest {};

TEST_F(ThemeColorMatchesManifestTest, ThemeColor) {
  Testcase testcases[] = {
      Testcase("theme_color.json"),
  };
  RunTestcases(testcases, arraysize(testcases), EXPECT_TYPE_SUCCESS);
}

}  // namespace extensions
