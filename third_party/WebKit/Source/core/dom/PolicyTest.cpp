#include "core/dom/Policy.h"

#include <memory>
#include "core/dom/Document.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#define SELF_ORIGIN "https://selforigin.com"
#define ORIGIN_A "https://example.com"
#define ORIGIN_B "https://example.net"

namespace blink {
using ::testing::UnorderedElementsAre;

class PolicyTest : public ::testing::Test {
 public:
  void SetUp() override {
    document_ = Document::CreateForTest();
    RefPtr<SecurityOrigin> self_origin =
        SecurityOrigin::CreateFromString(SELF_ORIGIN);
    document_->SetSecurityOrigin(self_origin);
    document_->SetFeaturePolicy(
        "fullscreen *; payment 'self'; midi 'none'; camera 'self' " ORIGIN_A
        " " ORIGIN_B "");
    policy_ = Policy::Create(document_);
  }

  Policy* GetPolicy() const { return policy_; }

 private:
  Persistent<Document> document_;
  Persistent<Policy> policy_;
};

TEST_F(PolicyTest, TestAllowsFeature) {
  EXPECT_FALSE(GetPolicy()->allowsFeature("badfeature"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("midi"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("midi", SELF_ORIGIN));
  EXPECT_TRUE(GetPolicy()->allowsFeature("fullscreen"));
  EXPECT_TRUE(GetPolicy()->allowsFeature("fullscreen", ORIGIN_A));
  EXPECT_TRUE(GetPolicy()->allowsFeature("payment"));
  EXPECT_FALSE(GetPolicy()->allowsFeature("payment", ORIGIN_A));
  EXPECT_FALSE(GetPolicy()->allowsFeature("payment", ORIGIN_B));
  EXPECT_TRUE(GetPolicy()->allowsFeature("camera"));
  EXPECT_TRUE(GetPolicy()->allowsFeature("camera", ORIGIN_A));
  EXPECT_TRUE(GetPolicy()->allowsFeature("camera", ORIGIN_B));
  EXPECT_FALSE(GetPolicy()->allowsFeature("camera", "https://badorigin.com"));
}

TEST_F(PolicyTest, TestGetAllowList) {
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("camera"),
              UnorderedElementsAre(SELF_ORIGIN, ORIGIN_A, ORIGIN_B));
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("payment"),
              UnorderedElementsAre(SELF_ORIGIN));
  EXPECT_THAT(GetPolicy()->getAllowlistForFeature("fullscreen"),
              UnorderedElementsAre("*"));
  EXPECT_TRUE(GetPolicy()->getAllowlistForFeature("badfeature").IsEmpty());
  EXPECT_TRUE(GetPolicy()->getAllowlistForFeature("midi").IsEmpty());
}

TEST_F(PolicyTest, TestAllowedFeatures) {
  Vector<String> allowed_features = GetPolicy()->allowedFeatures();
  EXPECT_TRUE(allowed_features.Contains("fullscreen"));
  EXPECT_TRUE(allowed_features.Contains("payment"));
  EXPECT_TRUE(allowed_features.Contains("camera"));
  // "geolocation" has default policy as allowed on self origin.
  EXPECT_TRUE(allowed_features.Contains("geolocation"));
  EXPECT_FALSE(allowed_features.Contains("badfeature"));
  EXPECT_FALSE(allowed_features.Contains("midi"));
}

}  // namespace blink
