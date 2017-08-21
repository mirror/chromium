// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/feature_policy/FeaturePolicy.h"

#include "platform/RuntimeEnabledFeatures.h"
#include "testing/gtest/include/gtest/gtest.h"

// Origin strings used for tests
#define ORIGIN_A "https://example.com/"
#define ORIGIN_B "https://example.net/"
#define ORIGIN_C "https://example.org/"

namespace blink {

namespace {

const char* const kValidPolicies[] = {
    ";;",  // Empty policies.
    "vibrate 'none'",
    "vibrate 'self'",
    "vibrate 'src'",  // Only valid for iframe allow attribute.
    "vibrate",        // Only valid for iframe allow attribute.
    "vibrate *",
    "vibrate " ORIGIN_A "",
    "vibrate " ORIGIN_B "",
    "vibrate  " ORIGIN_A " " ORIGIN_B "",
    "vibrate 'none' " ORIGIN_A " " ORIGIN_B "",
    "vibrate " ORIGIN_A " 'none' " ORIGIN_B "",
    "vibrate 'none' 'none' 'none'",
    "vibrate " ORIGIN_A " *",
    "fullscreen  " ORIGIN_A "; payment 'self'",
    "fullscreen " ORIGIN_A "; payment *, vibrate 'self'"};

const char* const kInvalidPolicies[] = {
    "badfeaturename",
    "badfeaturename 'self'",
    "1.0",
    "vibrate badorigin",
    "vibrate bad;origin",
    "vibrate bad,origin",
    "vibrate https://exmple.com, https://a.com",
    "vibrate *, payment badorigin"};

}  // namespace

class FeaturePolicyTest : public ::testing::Test {
 protected:
  FeaturePolicyTest() {}

  ~FeaturePolicyTest() {
  }

  RefPtr<SecurityOrigin> origin_a_ = SecurityOrigin::CreateFromString(ORIGIN_A);
  RefPtr<SecurityOrigin> origin_b_ = SecurityOrigin::CreateFromString(ORIGIN_B);
  RefPtr<SecurityOrigin> origin_c_ = SecurityOrigin::CreateFromString(ORIGIN_C);

  const FeatureNameMap test_feature_name_map = {
      {"fullscreen", blink::WebFeaturePolicyFeature::kFullscreen},
      {"payment", blink::WebFeaturePolicyFeature::kPayment},
      {"vibrate", blink::WebFeaturePolicyFeature::kVibrate}};
};

TEST_F(FeaturePolicyTest, ParseValidPolicy) {
  Vector<String> messages;
  for (const char* policy_string : kValidPolicies) {
    messages.clear();
    ParseContainerPolicy(policy_string, origin_a_.Get(), origin_b_.Get(),
                         &messages, test_feature_name_map);
    EXPECT_EQ(0UL, messages.size());
  }
}

TEST_F(FeaturePolicyTest, ParseInvalidPolicy) {
  Vector<String> messages;
  for (const char* policy_string : kInvalidPolicies) {
    messages.clear();
    ParseContainerPolicy(policy_string, origin_a_.Get(), origin_b_.Get(),
                         &messages, test_feature_name_map);
    EXPECT_NE(0UL, messages.size());
  }
}

TEST_F(FeaturePolicyTest, PolicyParsedCorrectly) {
  Vector<String> messages;

  // Empty policy.
  WebParsedFeaturePolicy parsed_policy = ParseContainerPolicy(
      "", origin_a_.Get(), origin_b_.Get(), &messages, test_feature_name_map);
  EXPECT_EQ(0UL, parsed_policy.size());

  // Simple policy with 'self'.
  // policy = "vibrate 'self'"
  parsed_policy =
      ParseContainerPolicy(kValidPolicies[2], origin_a_.Get(), origin_b_.Get(),
                           &messages, test_feature_name_map);
  EXPECT_EQ(1UL, parsed_policy.size());

  EXPECT_EQ(WebFeaturePolicyFeature::kVibrate, parsed_policy[0].feature);
  EXPECT_FALSE(parsed_policy[0].matches_all_origins);
  EXPECT_EQ(1UL, parsed_policy[0].origins.size());
  EXPECT_TRUE(origin_a_->IsSameSchemeHostPortAndSuborigin(
      parsed_policy[0].origins[0].Get()));
  // Simple policy with *.
  // policy = "vibrate *"
  parsed_policy =
      ParseContainerPolicy(kValidPolicies[5], origin_a_.Get(), origin_b_.Get(),
                           &messages, test_feature_name_map);
  EXPECT_EQ(1UL, parsed_policy.size());
  EXPECT_EQ(WebFeaturePolicyFeature::kVibrate, parsed_policy[0].feature);
  EXPECT_TRUE(parsed_policy[0].matches_all_origins);
  EXPECT_EQ(0UL, parsed_policy[0].origins.size());

  // Complicated policy.
  parsed_policy = ParseContainerPolicy(
      "vibrate *; "
      "fullscreen https://example.net https://example.org; "
      "payment 'self'",
      origin_a_.Get(), origin_b_.Get(), &messages, test_feature_name_map);
  EXPECT_EQ(3UL, parsed_policy.size());
  EXPECT_EQ(WebFeaturePolicyFeature::kVibrate, parsed_policy[0].feature);
  EXPECT_TRUE(parsed_policy[0].matches_all_origins);
  EXPECT_EQ(0UL, parsed_policy[0].origins.size());
  EXPECT_EQ(WebFeaturePolicyFeature::kFullscreen, parsed_policy[1].feature);
  EXPECT_FALSE(parsed_policy[1].matches_all_origins);
  EXPECT_EQ(2UL, parsed_policy[1].origins.size());
  EXPECT_TRUE(origin_b_->IsSameSchemeHostPortAndSuborigin(
      parsed_policy[1].origins[0].Get()));
  EXPECT_TRUE(origin_c_->IsSameSchemeHostPortAndSuborigin(
      parsed_policy[1].origins[1].Get()));
  EXPECT_EQ(WebFeaturePolicyFeature::kPayment, parsed_policy[2].feature);
  EXPECT_FALSE(parsed_policy[2].matches_all_origins);
  EXPECT_EQ(1UL, parsed_policy[2].origins.size());
  EXPECT_TRUE(origin_a_->IsSameSchemeHostPortAndSuborigin(
      parsed_policy[2].origins[0].Get()));

  // Multiple policies.
  parsed_policy = ParseContainerPolicy(
      "vibrate * https://exmple.net; "
      "fullscreen https://example.net none https://example.org,"
      "payment 'self' badorigin",
      origin_a_.Get(), origin_b_.Get(), &messages, test_feature_name_map);
  EXPECT_EQ(3UL, parsed_policy.size());
  EXPECT_EQ(WebFeaturePolicyFeature::kVibrate, parsed_policy[0].feature);
  EXPECT_TRUE(parsed_policy[0].matches_all_origins);
  EXPECT_EQ(0UL, parsed_policy[0].origins.size());
  EXPECT_EQ(WebFeaturePolicyFeature::kFullscreen, parsed_policy[1].feature);
  EXPECT_FALSE(parsed_policy[1].matches_all_origins);
  EXPECT_EQ(2UL, parsed_policy[1].origins.size());
  EXPECT_TRUE(origin_b_->IsSameSchemeHostPortAndSuborigin(
      parsed_policy[1].origins[0].Get()));
  EXPECT_TRUE(origin_c_->IsSameSchemeHostPortAndSuborigin(
      parsed_policy[1].origins[1].Get()));
  EXPECT_EQ(WebFeaturePolicyFeature::kPayment, parsed_policy[2].feature);
  EXPECT_FALSE(parsed_policy[2].matches_all_origins);
  EXPECT_EQ(1UL, parsed_policy[2].origins.size());
  EXPECT_TRUE(origin_a_->IsSameSchemeHostPortAndSuborigin(
      parsed_policy[2].origins[0].Get()));
}

}  // namespace blink
