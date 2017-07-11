// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings_utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace settings_utils {

TEST(SettingsUtilsTest, FixUpAndValidateStartupPage) {
  EXPECT_FALSE(FixUpAndValidateStartupPage(std::string(), nullptr));
  EXPECT_FALSE(FixUpAndValidateStartupPage("   ", nullptr));
  EXPECT_FALSE(FixUpAndValidateStartupPage("^&*@)^)", nullptr));
  EXPECT_FALSE(FixUpAndValidateStartupPage("chrome://quit", nullptr));

  EXPECT_TRUE(FixUpAndValidateStartupPage("facebook.com", nullptr));
  EXPECT_TRUE(FixUpAndValidateStartupPage("http://reddit.com", nullptr));
  EXPECT_TRUE(FixUpAndValidateStartupPage("https://google.com", nullptr));
  EXPECT_TRUE(FixUpAndValidateStartupPage("chrome://apps", nullptr));

  GURL fixed_url;
  EXPECT_TRUE(FixUpAndValidateStartupPage("about:settings", &fixed_url));
  EXPECT_EQ("chrome://settings/", fixed_url.spec());
}

}  // namespace settings_utils
