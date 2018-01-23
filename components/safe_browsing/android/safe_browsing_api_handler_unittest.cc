// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/android/safe_browsing_api_handler_util.h"

#include <string>

#include "base/strings/stringprintf.h"
#include "components/safe_browsing/db/metadata.pb.h"
#include "components/safe_browsing/db/util.h"
#include "components/safe_browsing/db/v4_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

class SafeBrowsingApiHandlerUtilTest : public ::testing::Test {
 protected:
  SBThreatType threat_;
  ThreatMetadata meta_;
  const ThreatMetadata empty_meta_;

  UmaRemoteCallResult ResetAndParseJson(const std::string& json) {
    threat_ = SB_THREAT_TYPE_EXTENSION;  // Should never be seen
    meta_ = ThreatMetadata();
    return ParseJsonFromGMSCore(json, &threat_, &meta_);
  }
};

TEST_F(SafeBrowsingApiHandlerUtilTest, BadJson) {
  EXPECT_EQ(UMA_STATUS_JSON_EMPTY, ResetAndParseJson(""));
  EXPECT_EQ(SB_THREAT_TYPE_SAFE, threat_);
  EXPECT_EQ(empty_meta_, meta_);

  EXPECT_EQ(UMA_STATUS_JSON_FAILED_TO_PARSE, ResetAndParseJson("{"));
  EXPECT_EQ(SB_THREAT_TYPE_SAFE, threat_);
  EXPECT_EQ(empty_meta_, meta_);

  EXPECT_EQ(UMA_STATUS_JSON_FAILED_TO_PARSE, ResetAndParseJson("[]"));
  EXPECT_EQ(SB_THREAT_TYPE_SAFE, threat_);
  EXPECT_EQ(empty_meta_, meta_);

  EXPECT_EQ(UMA_STATUS_JSON_FAILED_TO_PARSE,
            ResetAndParseJson("{\"matches\":\"foo\"}"));
  EXPECT_EQ(SB_THREAT_TYPE_SAFE, threat_);
  EXPECT_EQ(empty_meta_, meta_);

  EXPECT_EQ(UMA_STATUS_JSON_UNKNOWN_THREAT,
            ResetAndParseJson("{\"matches\":[{}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_SAFE, threat_);
  EXPECT_EQ(empty_meta_, meta_);

  EXPECT_EQ(UMA_STATUS_JSON_UNKNOWN_THREAT,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"junk\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_SAFE, threat_);
  EXPECT_EQ(empty_meta_, meta_);

  EXPECT_EQ(UMA_STATUS_JSON_UNKNOWN_THREAT,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"999\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_SAFE, threat_);
  EXPECT_EQ(empty_meta_, meta_);
}

TEST_F(SafeBrowsingApiHandlerUtilTest, BasicThreats) {
  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"4\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE, threat_);
  EXPECT_EQ(empty_meta_, meta_);

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"5\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_PHISHING, threat_);
  EXPECT_EQ(empty_meta_, meta_);
}

TEST_F(SafeBrowsingApiHandlerUtilTest, MultipleThreats) {
  EXPECT_EQ(
      UMA_STATUS_MATCH,
      ResetAndParseJson(
          "{\"matches\":[{\"threat_type\":\"4\"}, {\"threat_type\":\"5\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE, threat_);
  EXPECT_EQ(empty_meta_, meta_);
}

TEST_F(SafeBrowsingApiHandlerUtilTest, PhaSubType) {
  ThreatMetadata expected;

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"4\", "
                              "\"pha_pattern_type\":\"LANDING\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE, threat_);
  expected.threat_pattern_type = ThreatPatternType::MALWARE_LANDING;
  EXPECT_EQ(expected, meta_);
  // Test the ThreatMetadata comparitor for this field.
  EXPECT_NE(empty_meta_, meta_);

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"4\", "
                              "\"pha_pattern_type\":\"DISTRIBUTION\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE, threat_);
  expected.threat_pattern_type = ThreatPatternType::MALWARE_DISTRIBUTION;
  EXPECT_EQ(expected, meta_);

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"4\", "
                              "\"pha_pattern_type\":\"junk\"}]}"));
  EXPECT_EQ(empty_meta_, meta_);
}

TEST_F(SafeBrowsingApiHandlerUtilTest, SocialEngineeringSubType) {
  ThreatMetadata expected;

  EXPECT_EQ(
      UMA_STATUS_MATCH,
      ResetAndParseJson("{\"matches\":[{\"threat_type\":\"5\", "
                        "\"se_pattern_type\":\"SOCIAL_ENGINEERING_ADS\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_PHISHING, threat_);
  expected.threat_pattern_type = ThreatPatternType::SOCIAL_ENGINEERING_ADS;
  EXPECT_EQ(expected, meta_);

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson(
                "{\"matches\":[{\"threat_type\":\"5\", "
                "\"se_pattern_type\":\"SOCIAL_ENGINEERING_LANDING\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_PHISHING, threat_);
  expected.threat_pattern_type = ThreatPatternType::SOCIAL_ENGINEERING_LANDING;
  EXPECT_EQ(expected, meta_);

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"5\", "
                              "\"se_pattern_type\":\"PHISHING\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_PHISHING, threat_);
  expected.threat_pattern_type = ThreatPatternType::PHISHING;
  EXPECT_EQ(expected, meta_);

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"5\", "
                              "\"se_pattern_type\":\"junk\"}]}"));
  EXPECT_EQ(empty_meta_, meta_);
}

TEST_F(SafeBrowsingApiHandlerUtilTest, PopulationId) {
  ThreatMetadata expected;

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"4\", "
                              "\"UserPopulation\":\"foobarbazz\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_MALWARE, threat_);
  expected.population_id = "foobarbazz";
  EXPECT_EQ(expected, meta_);
  // Test the ThreatMetadata comparator for this field.
  EXPECT_NE(empty_meta_, meta_);
}

TEST_F(SafeBrowsingApiHandlerUtilTest, NoSubresourceFilterSubTypes) {
  ThreatMetadata expected;

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"13\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_SUBRESOURCE_FILTER, threat_);
  expected.threat_pattern_type = ThreatPatternType::NONE;
  EXPECT_EQ(expected, meta_);

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"13\", "
                              "\"se_pattern_type\":\"junk\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_SUBRESOURCE_FILTER, threat_);
  expected.threat_pattern_type = ThreatPatternType::NONE;
  EXPECT_EQ(expected, meta_);
}

TEST_F(SafeBrowsingApiHandlerUtilTest, SubresourceFilterSubTypes) {
  typedef SubresourceFilterLevel Level;
  typedef SubresourceFilterType Type;
  // If possible, keep these test cases identical to
  // V4GetHashProtocolManagerTest.TestParseSubresourceFilterMetadata.
  const struct {
    std::vector<std::pair<const char*, const char*>> metadata;
    SubresourceFilterMatch expected_match;
  } test_cases[] = {
      {{}, {}},
      {{{"bleh", "asdf"}}, {}},
      {{{"nothing", ""}, {"stillnothing", ""}}, {}},
      {{{"a", ""}}, {{{Type::ABUSIVE, Level::ENFORCE}}}},
      {{{"b", ""}}, {{{Type::BETTER_ADS, Level::ENFORCE}}}},
      // sf_* entries have precedence (arbitrary).
      {{{"a", ""}, {"sf_absv", "warn"}}, {{{Type::ABUSIVE, Level::WARN}}}},
      {{{"b", ""}, {"sf_bas", "warn"}}, {{{Type::BETTER_ADS, Level::WARN}}}},
      {{{"a", ""}, {"b", "enforce"}},
       {{{Type::ABUSIVE, Level::ENFORCE}, {Type::BETTER_ADS, Level::ENFORCE}}}},
      {{{"a", "warn"}}, {{{Type::ABUSIVE, Level::WARN}}}},
      {{{"b", "warn"}}, {{{Type::BETTER_ADS, Level::WARN}}}},
      {{{"a", "warn"}, {"b", "warn"}},
       {{{Type::ABUSIVE, Level::WARN}, {Type::BETTER_ADS, Level::WARN}}}},
      {{{"a", "enforce"}, {"b", "warn"}},
       {{{Type::ABUSIVE, Level::ENFORCE}, {Type::BETTER_ADS, Level::WARN}}}},
      {{{"sf_absv", ""}}, {{{Type::ABUSIVE, Level::ENFORCE}}}},
      {{{"sf_bas", ""}}, {{{Type::BETTER_ADS, Level::ENFORCE}}}},
      {{{"sf_absv", ""}, {"sf_bas", "enforce"}},
       {{{Type::ABUSIVE, Level::ENFORCE}, {Type::BETTER_ADS, Level::ENFORCE}}}},
      {{{"sf_absv", "warn"}}, {{{Type::ABUSIVE, Level::WARN}}}},
      {{{"sf_bas", "warn"}}, {{{Type::BETTER_ADS, Level::WARN}}}},
      {{{"sf_absv", "warn"}, {"sf_bas", "warn"}},
       {{{Type::ABUSIVE, Level::WARN}, {Type::BETTER_ADS, Level::WARN}}}},
      {{{"sf_absv", "enforce"}, {"sf_bas", "warn"}},
       {{{Type::ABUSIVE, Level::ENFORCE}, {Type::BETTER_ADS, Level::WARN}}}},
  };

  for (const auto& test_case : test_cases) {
    std::string json = R"({
        "matches" : [{
          "threat_type":"13"
          %s
        }]
      })";
    std::string metadata;
    for (auto it : test_case.metadata) {
      std::string entry =
          base::StringPrintf(",\"%s\":\"%s\"", it.first, it.second);
      metadata += entry;
    }
    json = base::StringPrintf(json.c_str(), metadata.c_str());
    SCOPED_TRACE(testing::Message() << json);

    ASSERT_EQ(UMA_STATUS_MATCH, ResetAndParseJson(json));
    EXPECT_EQ(SB_THREAT_TYPE_SUBRESOURCE_FILTER, threat_);

    ThreatMetadata expected;
    expected.subresource_filter_match = test_case.expected_match;
    EXPECT_EQ(expected, meta_);
  }
}

TEST_F(SafeBrowsingApiHandlerUtilTest, NoUnwantedSoftwareSubTypes) {
  ThreatMetadata expected;

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"3\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_UNWANTED, threat_);
  expected.threat_pattern_type = ThreatPatternType::NONE;
  EXPECT_EQ(expected, meta_);

  EXPECT_EQ(UMA_STATUS_MATCH,
            ResetAndParseJson("{\"matches\":[{\"threat_type\":\"3\", "
                              "\"se_pattern_type\":\"junk\"}]}"));
  EXPECT_EQ(SB_THREAT_TYPE_URL_UNWANTED, threat_);
  expected.threat_pattern_type = ThreatPatternType::NONE;
  EXPECT_EQ(expected, meta_);
}

}  // namespace safe_browsing
