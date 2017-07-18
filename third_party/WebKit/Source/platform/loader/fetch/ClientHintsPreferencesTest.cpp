// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/ClientHintsPreferences.h"

#include "platform/loader/fetch/ResourceResponse.h"
#include "platform/weborigin/KURL.h"
#include "public/platform/WebRuntimeFeatures.h"
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
              preferences.ShouldSend(kWebClientHintsTypeDpr));
    EXPECT_EQ(test_case.expectation_viewport_width,
              preferences.ShouldSend(kWebClientHintsTypeViewportWidth));

    // Calling UpdateFromAcceptClientHintsHeader with empty header should have
    // no impact on client hint preferences.
    preferences.UpdateFromAcceptClientHintsHeader("", nullptr);
    EXPECT_EQ(test_case.expectation_resource_width,
              preferences.ShouldSend(kWebClientHintsTypeResourceWidth));
    EXPECT_EQ(test_case.expectation_dpr,
              preferences.ShouldSend(kWebClientHintsTypeDpr));
    EXPECT_EQ(test_case.expectation_viewport_width,
              preferences.ShouldSend(kWebClientHintsTypeViewportWidth));

    // Calling UpdateFromAcceptClientHintsHeader with an invalid header should
    // have no impact on client hint preferences.
    preferences.UpdateFromAcceptClientHintsHeader("foobar", nullptr);
    EXPECT_EQ(test_case.expectation_resource_width,
              preferences.ShouldSend(kWebClientHintsTypeResourceWidth));
    EXPECT_EQ(test_case.expectation_dpr,
              preferences.ShouldSend(kWebClientHintsTypeDpr));
    EXPECT_EQ(test_case.expectation_viewport_width,
              preferences.ShouldSend(kWebClientHintsTypeViewportWidth));
  }
}

TEST_F(ClientHintsPreferencesTest, PersistentHints) {
  struct TestCase {
    bool enable_persistent_runtime_feature;
    bool use_https_url;
    const char* accept_ch_header_value;
    const char* accept_lifetime_header_value;
    int64_t expect_persist_duration_seconds;
  } test_cases[] = {
      {true, true, "width, dpr, viewportWidth", "", -1},
      {true, true, "width, dpr, viewportWidth", "-1000", -1},
      {true, true, "width, dpr, viewportWidth", "1000s", -1},
      {true, true, "width, dpr, viewportWidth", "1000.5", -1},
      {false, true, "width, dpr, viewportWidth", "1000", -1},
      {true, false, "width, dpr, viewportWidth", "1000", -1},
      {true, true, "width, dpr, viewportWidth", "1000", 1000},
  };

  for (const auto& test : test_cases) {
    WebRuntimeFeatures::EnableClientHintsPersistent(
        test.enable_persistent_runtime_feature);
    ClientHintsPreferences preferences;
    int64_t persist_duration_seconds = 0;

    const KURL kurl(kParsedURLString,
                    test.use_https_url
                        ? String::FromUTF8("https://www.google.com/")
                        : String::FromUTF8("http://www.google.com/"));

    ResourceResponse response;
    response.SetHTTPHeaderField("accept-ch", test.accept_ch_header_value);
    response.SetHTTPHeaderField("accept-ch-lifetime",
                                test.accept_lifetime_header_value);
    response.SetURL(kurl);

    preferences.UpdatePersistentHintsFromHeaders(response, nullptr,
                                                 &persist_duration_seconds);
    EXPECT_EQ(test.expect_persist_duration_seconds, persist_duration_seconds);
    if (persist_duration_seconds > 0) {
      EXPECT_FALSE(preferences.ShouldSend(kWebClientHintsTypeDeviceRam));
      EXPECT_TRUE(preferences.ShouldSend(kWebClientHintsTypeDpr));
      EXPECT_TRUE(preferences.ShouldSend(kWebClientHintsTypeResourceWidth));
      EXPECT_FALSE(preferences.ShouldSend(kWebClientHintsTypeViewportWidth));
    } else {
      EXPECT_FALSE(preferences.ShouldSend(kWebClientHintsTypeDeviceRam));
      EXPECT_FALSE(preferences.ShouldSend(kWebClientHintsTypeDpr));
      EXPECT_FALSE(preferences.ShouldSend(kWebClientHintsTypeResourceWidth));
      EXPECT_FALSE(preferences.ShouldSend(kWebClientHintsTypeViewportWidth));
    }
  }
}

}  // namespace blink
