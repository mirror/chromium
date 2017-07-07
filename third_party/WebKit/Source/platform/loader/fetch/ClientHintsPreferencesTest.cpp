// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/ClientHintsPreferences.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class ClientHintsPreferencesTest : public ::testing::Test {};

TEST_F(ClientHintsPreferencesTest, Basic) {
  struct TestCase {
    const char* header_value;
    bool expectation_resource_width;
    bool expectation_dpr;
    bool expectation_viewport_width;
  } cases[] = {
      {"width, dpr, viewportWidth", true, true, false},
      {"WiDtH, dPr,     viewport-width", true, true, true},
      {"WIDTH, DPR, VIWEPROT-Width", true, true, false},
      {"VIewporT-Width, wutwut, width", true, false, true},
      {"dprw", false, false, false},
      {"DPRW", false, false, false},
  };

  for (const auto& test_case : cases) {
    ClientHintsPreferences preferences;
    preferences.UpdateFromAcceptClientHintsHeader(test_case.header_value,
                                                  nullptr);
    EXPECT_EQ(test_case.expectation_resource_width,
              preferences.ShouldSend(kWebClientHintsTypeResourceWidth));
    EXPECT_EQ(test_case.expectation_dpr,
              preferences.ShouldSend(kWebClientHintsTypeDPR));
    EXPECT_EQ(test_case.expectation_viewport_width,
              preferences.ShouldSend(kWebClientHintsTypeViewportWidth));
  }
}

TEST_F(ClientHintsPreferencesTest, Persistence) {
  struct TestCase {
    const char* accept_ch_header_value;
    const char* accept_lifetime_header_value;
    int64_t expect_persist_duration_seconds;
  } cases[] = {
      {"width, dpr, viewportWidth", "", -1},
      {"width, dpr, viewportWidth", "1000", 1000},
      {"width, dpr, viewportWidth", "-1000", -1},
      {"width, dpr, viewportWidth", "1000s", -1},
      {"width, dpr, viewportWidth", "1000.5", -1},
  };

  for (const auto& test_case : cases) {
    ClientHintsPreferences preferences;
    bool enabled_types[kWebClientHintsTypeNumValues] = {false};
    int64_t persist_duration_seconds = 0;

    preferences.UpdatePersistentHintsFromHeaders(
        test_case.accept_ch_header_value,
        test_case.accept_lifetime_header_value, enabled_types,
        &persist_duration_seconds);
    EXPECT_EQ(test_case.expect_persist_duration_seconds,
              persist_duration_seconds);
    if (persist_duration_seconds > 0) {
      EXPECT_FALSE(enabled_types[kWebClientHintsTypeDeviceRam]);
      EXPECT_TRUE(enabled_types[kWebClientHintsTypeDPR]);
      EXPECT_TRUE(enabled_types[kWebClientHintsTypeResourceWidth]);
      EXPECT_FALSE(enabled_types[kWebClientHintsTypeViewportWidth]);
    } else {
      EXPECT_FALSE(enabled_types[kWebClientHintsTypeDeviceRam]);
      EXPECT_FALSE(enabled_types[kWebClientHintsTypeDPR]);
      EXPECT_FALSE(enabled_types[kWebClientHintsTypeResourceWidth]);
      EXPECT_FALSE(enabled_types[kWebClientHintsTypeViewportWidth]);
    }
  }
}

}  // namespace blink
