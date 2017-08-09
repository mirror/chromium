// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_amp_converter.h"

#include <memory>
#include <string>

#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace previews {

class PreviewsAMPConverterTest : public testing::Test {
 public:
  PreviewsAMPConverterTest() : field_trial_list_(nullptr) {}

  void Initialize() { amp_converter_.reset(new PreviewsAMPConverter()); }

  PreviewsAMPConverter* amp_converter() { return amp_converter_.get(); }

 private:
  base::FieldTrialList field_trial_list_;
  std::unique_ptr<PreviewsAMPConverter> amp_converter_;
};

void CreateFieldTrialWithParams(
    const std::string& trial_name,
    const std::string& group_name,
    std::initializer_list<
        typename std::map<std::string, std::string>::value_type> params) {
  EXPECT_TRUE(base::AssociateFieldTrialParams(trial_name, group_name, params));
  EXPECT_TRUE(base::FieldTrialList::CreateFieldTrial(trial_name, group_name));
}

TEST_F(PreviewsAMPConverterTest, TestDisallowedByDefault) {
  Initialize();

  const char* urls[] = {"", "http://test.com", "https://test.com",
                        "http://www.test.com", "http://test.com/index.html"};
  GURL amp_url;
  for (const char* url : urls) {
    EXPECT_FALSE(amp_converter()->GetAMPURL(GURL(url), &amp_url)) << url;
  }
}

TEST_F(PreviewsAMPConverterTest, TestFieldTrialEnabled) {
  struct {
    const char* url;
    bool expected_result;
    const char* expected_amp_url;
  } tests[] = {
      {"", false},
      {"http://www.test.com", false},
      {"http://test.com", true, "http://test.com/"},
      {"https://test.com", true, "https://test.com/"},
      {"http://test.com/index.html", true, "http://test.com/index.html"}};

  CreateFieldTrialWithParams(
      "AMPRedirection", "Enabled",
      {{"config", "[{\"host\":\"test.com\", \"pattern\":\".*\"}]"}});
  Initialize();

  for (const auto& test : tests) {
    GURL amp_url;
    EXPECT_EQ(test.expected_result,
              amp_converter()->GetAMPURL(GURL(test.url), &amp_url))
        << test.url;
    if (test.expected_amp_url)
      EXPECT_EQ(test.expected_amp_url, amp_url.spec());
  }
}

}  // namespace previews
