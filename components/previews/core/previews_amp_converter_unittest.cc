// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_amp_converter.h"

#include <memory>
#include <string>

#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "components/variations/variations_associated_data.h"
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
  variations::testing::ClearAllVariationParams();
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
      {"http://test.com/index.html", true, "http://test.com/index.html"},
  };

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

TEST_F(PreviewsAMPConverterTest, TestMultipleConversions) {
  CreateFieldTrialWithParams(
      "AMPRedirection", "Enabled",
      {{"config",
        "[{\"host\":\"test.com\", \"pattern\":\".*\", "
        "\"hostamp\":\"amp.test.com\"},"
        "{\"host\":\"testprefix.com\", \"pattern\":\".*\", "
        "\"prefix\":\"/amp\"},"
        "{\"host\":\"testsuffix.com\", \"pattern\":\".*\", "
        "\"suffix\":\".amp\"},"
        "{\"host\":\"testsuffixhtml.com\", \"pattern\":\".*\", "
        "\"suffixhtml\":\".amp\"},"
        "{\"host\":\"prefixandsuffix.com\", \"pattern\":\".*\", "
        "\"prefix\":\"/pre\", \"suffix\":\".suf\"}]"}});
  Initialize();

  struct {
    const char* url;
    bool expected_result;
    const char* expected_amp_url;
  } tests[] = {
      {"", false},
      {"http://www.test.com", false},
      {"http://test.com", true, "http://amp.test.com/"},
      {"https://test.com", true, "https://amp.test.com/"},
      {"http://test.com/index.html?id=foo#anchor", true,
       "http://amp.test.com/index.html?id=foo#anchor"},
      {"http://testprefix.com/index.html?id=foo#anchor", true,
       "http://testprefix.com/amp/index.html?id=foo#anchor"},
      {"http://testsuffix.com/index.html?id=foo#anchor", true,
       "http://testsuffix.com/index.html.amp?id=foo#anchor"},
      {"http://testsuffixhtml.com/index.html?id=foo#anchor", true,
       "http://testsuffixhtml.com/index.amp.html?id=foo#anchor"},
      {"http://prefixandsuffix.com/index.html?id=foo#anchor", true,
       "http://prefixandsuffix.com/pre/index.html."
       "suf?id=foo#anchor"},
  };

  for (const auto& test : tests) {
    GURL amp_url;
    EXPECT_EQ(test.expected_result,
              amp_converter()->GetAMPURL(GURL(test.url), &amp_url))
        << test.url;
    if (test.expected_amp_url)
      EXPECT_EQ(test.expected_amp_url, amp_url.spec());
  }
}

TEST_F(PreviewsAMPConverterTest, TestURLPatterns) {
  CreateFieldTrialWithParams(
      "AMPRedirection", "Enabled",
      {{"config",
        "[{\"host\":\"test1.com\", \"pattern\":\"/foo/bar/.+\"},"
        "{\"host\":\"test2.com\", \"pattern\":\"/(foo|bar)/.*.html$\"},"
        "{\"host\":\"test3.com\", "
        "\"pattern\":\"/[0-9]{4}/[0-9]{2}/[0-9]{2}/.*.html$\"}]"}});
  Initialize();

  struct {
    const char* url;
    bool expected_result;
    const char* expected_amp_url;
  } tests[] = {
      {"http://test1.com", false},
      {"http://test1.com/foo/bar", false},
      {"http://test1.com/foo/bar/", false},
      {"http://test1.com/foo/bar/baz", true, "http://test1.com/foo/bar/baz"},
      {"http://test2.com/foo/index.html", true,
       "http://test2.com/foo/index.html"},
      {"http://test2.com/bar/b.html", true, "http://test2.com/bar/b.html"},
      {"http://test2.com/baz/b.html", false},
      {"http://test3.com/2017/01/02/index.html", true,
       "http://test3.com/2017/01/02/index.html"}};

  for (const auto& test : tests) {
    GURL amp_url;
    EXPECT_EQ(test.expected_result,
              amp_converter()->GetAMPURL(GURL(test.url), &amp_url))
        << test.url;
    if (test.expected_amp_url)
      EXPECT_EQ(test.expected_amp_url, amp_url.spec());
  }
}

TEST_F(PreviewsAMPConverterTest, TestURLScheme) {
  CreateFieldTrialWithParams(
      "AMPRedirection", "Enabled",
      {{"config",
        "[{\"host\":\"test_http.com\", \"scheme\":\"http\", "
        "\"pattern\":\".*\"},"
        "{\"host\":\"test_https.com\", \"scheme\":\"https\", "
        "\"pattern\":\".*\"},"
        "{\"host\":\"test_both.com\", \"pattern\":\".*\"},"
        "{\"host\":\"test_amp_http.com\", \"pattern\":\".*\", "
        "\"schemeamp\":\"http\"},"
        "{\"host\":\"test_amp_https.com\", \"pattern\":\".*\", "
        "\"schemeamp\":\"https\"},"
        "{\"host\":\"test_amp_invalid.com\", "
        "\"pattern\":\".*\", \"schemeamp\":\"both\"}]"}});
  Initialize();

  struct {
    const char* url;
    bool expected_result;
    const char* expected_amp_url;
  } tests[] = {
      {"http://test_http.com", true, "http://test_http.com/"},
      {"https://test_http.com", false},
      {"http://test_https.com", false},
      {"https://test_https.com", true, "https://test_https.com/"},
      {"http://test_both.com", true, "http://test_both.com/"},
      {"https://test_both.com", true, "https://test_both.com/"},
      {"http://test_amp_http.com", true, "http://test_amp_http.com/"},
      {"https://test_amp_http.com", true, "http://test_amp_http.com/"},
      {"http://test_amp_https.com", true, "https://test_amp_https.com/"},
      {"https://test_amp_https.com", true, "https://test_amp_https.com/"},
      {"https://test_amp_invalid.com", false},
      {"http://test_amp_invalid.com", false},
  };

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
