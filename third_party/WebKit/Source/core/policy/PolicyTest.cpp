// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/policy/Policy.h"

#include "core/dom/Document.h"
#include "platform/feature_policy/FeaturePolicy.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {
constexpr char kSelfOrigin[] = "https://selforigin.com";
constexpr char kOriginA[] = "https://example.com";
constexpr char kOriginB[] = "https://example.net";
}  // namespace

using ::testing::UnorderedElementsAre;

class PolicyTest : public ::testing::Test {
 public:
  void SetUp() override {
    document_ = Document::CreateForTest();
    document_->SetSecurityOrigin(SecurityOrigin::CreateFromString(kSelfOrigin));
    document_->ApplyFeaturePolicyFromHeader(
        "fullscreen *; payment 'self'; midi 'none'; camera 'self' "
        "https://example.com https://example.net");
  }

  Policy* GetPolicy() const { return policy_; }

 protected:
  Persistent<Document> document_;
  Persistent<Policy> policy_;
};

class DocumentPolicyTest : public PolicyTest {
 public:
  void SetUp() override {
    PolicyTest::SetUp();
    policy_ = Policy::Create(Policy::Type::DocumentPolicy, document_);
  }
};

class FramePolicyTest : public PolicyTest {
 public:
  void UpdateOrigin(const String& origin) {
    src_origin_ = SecurityOrigin::CreateFromString(origin);
  }

  void UpdateContainerPolicy(const ParsedFeaturePolicy& container_policy) {
    container_policy_ = container_policy;
  }

  void UpdatePolicy() {
    if (policy_) {
      policy_->UpdateContainerPolicy(&container_policy_, src_origin_);
      return;
    }
    policy_ = Policy::Create(Policy::Type::FramePolicy, document_,
                             &container_policy_, src_origin_);
  }

 private:
  scoped_refptr<SecurityOrigin> src_origin_;
  ParsedFeaturePolicy container_policy_;
};

TEST_F(DocumentPolicyTest, TestAllowsFeature) {
  EXPECT_FALSE(GetPolicy()->allowsFeature("badfeature"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("midi"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("midi", kSelfOrigin));
  EXPECT_TRUE(GetPolicy()->allowsFeature("fullscreen"));
  EXPECT_TRUE(GetPolicy()->allowsFeature("fullscreen", kOriginA));
  EXPECT_TRUE(GetPolicy()->allowsFeature("payment"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("payment", kOriginA));
  EXPECT_FALSE(GetPolicy()->allowsFeature("payment", kOriginB));
  EXPECT_TRUE(GetPolicy()->allowsFeature("camera"));
  EXPECT_TRUE(GetPolicy()->allowsFeature("camera", kOriginA));
  EXPECT_TRUE(GetPolicy()->allowsFeature("camera", kOriginB));
  EXPECT_FALSE(GetPolicy()->allowsFeature("camera", "https://badorigin.com"));
  EXPECT_TRUE(GetPolicy()->allowsFeature("geolocation", kSelfOrigin));
}

TEST_F(DocumentPolicyTest, TestGetAllowList) {
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("camera"),
              UnorderedElementsAre(kSelfOrigin, kOriginA, kOriginB));
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("payment"),
              UnorderedElementsAre(kSelfOrigin));
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("geolocation"),
              UnorderedElementsAre(kSelfOrigin));
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("fullscreen"),
              UnorderedElementsAre("*"));
  EXPECT_TRUE(GetPolicy()->getAllowlistForFeature("badfeature").IsEmpty());
  EXPECT_TRUE(GetPolicy()->getAllowlistForFeature("midi").IsEmpty());
}

TEST_F(DocumentPolicyTest, TestAllowedFeatures) {
  Vector<String> allowed_features = GetPolicy()->allowedFeatures();
  EXPECT_TRUE(allowed_features.Contains("fullscreen"));
  EXPECT_TRUE(allowed_features.Contains("payment"));
  EXPECT_TRUE(allowed_features.Contains("camera"));
  // "geolocation" has default policy as allowed on self origin.
  EXPECT_TRUE(allowed_features.Contains("geolocation"));
  EXPECT_FALSE(allowed_features.Contains("badfeature"));
  EXPECT_FALSE(allowed_features.Contains("midi"));
}

TEST_F(FramePolicyTest, TestAllowsFeature) {
  ParsedFeaturePolicy container_policy;
  UpdateOrigin(kSelfOrigin);
  UpdateContainerPolicy(container_policy);
  UpdatePolicy();
  EXPECT_FALSE(GetPolicy()->allowsFeature("badfeature"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("midi"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("midi", kSelfOrigin));
  EXPECT_TRUE(GetPolicy()->allowsFeature("fullscreen"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("fullscreen", kOriginA));
  EXPECT_TRUE(GetPolicy()->allowsFeature("fullscreen", kSelfOrigin));
  EXPECT_TRUE(GetPolicy()->allowsFeature("payment"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("payment", kOriginA));
  EXPECT_FALSE(GetPolicy()->allowsFeature("payment", kOriginB));
  EXPECT_TRUE(GetPolicy()->allowsFeature("camera"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("camera", kOriginA));
  EXPECT_FALSE(GetPolicy()->allowsFeature("camera", kOriginB));
  EXPECT_FALSE(GetPolicy()->allowsFeature("camera", "https://badorigin.com"));
  EXPECT_TRUE(GetPolicy()->allowsFeature("geolocation", kSelfOrigin));
}

TEST_F(FramePolicyTest, TestGetAllowList) {
  ParsedFeaturePolicy container_policy;
  UpdateOrigin(kSelfOrigin);
  UpdateContainerPolicy(container_policy);
  UpdatePolicy();
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("camera"),
              UnorderedElementsAre(kSelfOrigin));
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("payment"),
              UnorderedElementsAre(kSelfOrigin));
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("geolocation"),
              UnorderedElementsAre(kSelfOrigin));
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("fullscreen"),
              UnorderedElementsAre(kSelfOrigin));
  EXPECT_TRUE(GetPolicy()->getAllowlistForFeature("badfeature").IsEmpty());
  EXPECT_TRUE(GetPolicy()->getAllowlistForFeature("midi").IsEmpty());
}

TEST_F(FramePolicyTest, TestAllowedFeatures) {
  ParsedFeaturePolicy container_policy;
  UpdateOrigin(kSelfOrigin);
  UpdateContainerPolicy(container_policy);
  UpdatePolicy();
  Vector<String> allowed_features = GetPolicy()->allowedFeatures();
  EXPECT_TRUE(allowed_features.Contains("fullscreen"));
  EXPECT_TRUE(allowed_features.Contains("payment"));
  EXPECT_TRUE(allowed_features.Contains("camera"));
  // "geolocation" has default policy as allowed on self origin.
  EXPECT_TRUE(allowed_features.Contains("geolocation"));
  EXPECT_FALSE(allowed_features.Contains("badfeature"));
  EXPECT_FALSE(allowed_features.Contains("midi"));
}

TEST_F(FramePolicyTest, TestCombinedPolicy) {
  ParsedFeaturePolicy container_policy = ParseFeaturePolicyAttribute(
      "geolocation 'src'; payment 'none'; midi; camera 'src'",
      SecurityOrigin::CreateFromString(kSelfOrigin),
      SecurityOrigin::CreateFromString(kOriginA), nullptr, nullptr);
  UpdateOrigin(kOriginA);
  UpdateContainerPolicy(container_policy);
  UpdatePolicy();
  Vector<String> allowed_features = GetPolicy()->allowedFeatures();
  EXPECT_TRUE(allowed_features.Contains("fullscreen"));
  EXPECT_FALSE(allowed_features.Contains("payment"));
  EXPECT_TRUE(allowed_features.Contains("geolocation"));
  EXPECT_FALSE(allowed_features.Contains("midi"));
  EXPECT_TRUE(allowed_features.Contains("camera"));
  // "geolocation" has default policy as allowed on self origin.
  EXPECT_FALSE(allowed_features.Contains("badfeature"));
}

}  // namespace blink
