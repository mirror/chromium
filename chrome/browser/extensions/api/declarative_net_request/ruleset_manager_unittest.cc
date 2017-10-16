// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/ruleset_manager.h"

#include "extensions/browser/api/declarative_net_request/ruleset_matcher.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/profiles/profile.h"
#include "components/url_pattern_index/flat/url_pattern_index_generated.h"
#include "components/version_info/version_info.h"
#include "extensions/browser/api/declarative_net_request/test_utils.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/api/declarative_net_request/test_utils.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/file_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "extensions/browser/info_map.h"
#include "url/origin.h"
#include "net/url_request/url_request_test_util.h"
#include "extensions/browser/extension_system.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "extensions/browser/extension_util.h"
#include "chrome/browser/extensions/extension_util.h"
#include "content/public/test/test_utils.h"

namespace extensions {
namespace declarative_net_request {
namespace {

constexpr char kJSONRulesFilename[] = "rules_file.json";
const base::FilePath::CharType kJSONRulesetFilepath[] =
    FILE_PATH_LITERAL("rules_file.json");

class RulesetManagerTest
    : public ExtensionServiceTestBase,
      public ::testing::WithParamInterface<ExtensionLoadType> {
 public:
  // Use channel UNKNOWN to ensure that the declarativeNetRequest API is
  // available, irrespective of its actual availability.
  RulesetManagerTest() : channel_(::version_info::Channel::UNKNOWN) {}

  void SetUp() override {
    ExtensionServiceTestBase::SetUp();
    InitializeEmptyExtensionService();
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  std::unique_ptr<ChromeTestExtensionLoader> GetExtensionLoader() {
    auto loader = std::make_unique<ChromeTestExtensionLoader>(browser_context());
    switch (GetParam()) {
      case ExtensionLoadType::PACKED:
        loader->set_pack_extension(true);

        // CrxInstaller reloads the extension after moving it, which causes an
        // install warning for packed extensions due to the presence of
        // kMetadata folder. However, this isn't actually surfaced to the user.
        loader->set_ignore_manifest_warnings(true);
        break;
      case ExtensionLoadType::UNPACKED:
        loader->set_pack_extension(false);
        break;
    }
    return loader;
  }

 protected:
  void CreateMatcherForRules(
      const std::vector<TestRule>& rules,
      const std::string& extension_dirname,
      std::unique_ptr<RulesetMatcher>* matcher) {
    base::FilePath extension_dir = temp_dir_.GetPath().Append(extension_dirname);

    // Create extension directory.
    ASSERT_TRUE(base::CreateDirectory(extension_dir));
    WriteManifestAndRuleset(extension_dir, kJSONRulesetFilepath,
                            kJSONRulesFilename, rules);

    // The Extension shared pointer will still be held by the Extension system.
    // Hence it's ok to not keep it alive here.
    last_loaded_extension_ = GetExtensionLoader()->LoadExtension(extension_dir);
    ASSERT_TRUE(last_loaded_extension_);

    // Simulate updating InfoMap as well. This won't happen automatically since
    // ExtensionSystem is mocked in tests.

    int expected_checksum;
    EXPECT_TRUE(
        ExtensionPrefs::Get(browser_context())
            ->GetDNRRulesetChecksum(last_loaded_extension_->id(), &expected_checksum));

    EXPECT_EQ(RulesetMatcher::LOAD_SUCCESS,
              RulesetMatcher::CreateVerifiedMatcher(
                  file_util::GetIndexedRulesetPath(last_loaded_extension_->path()),
                  expected_checksum, matcher));
  }

  InfoMap* info_map() {
    return ExtensionSystem::Get(browser_context())->info_map();
  }

  const Extension* last_loaded_extension() const {
    return last_loaded_extension_.get();
  }

  std::unique_ptr<net::URLRequest> GetRequestForURL(const std::string url) {
    return request_context_.CreateRequest(GURL(url), net::DEFAULT_PRIORITY, &delegate_,
                                  TRAFFIC_ANNOTATION_FOR_TESTS);
  }

 private:
  ScopedCurrentChannel channel_;
  base::ScopedTempDir temp_dir_;
  net::TestURLRequestContext request_context_;
  net::TestDelegate delegate_;
  scoped_refptr<const Extension> last_loaded_extension_;

  DISALLOW_COPY_AND_ASSIGN(RulesetManagerTest);
};

TEST_P(RulesetManagerTest, MultipleRulesets) {
  TestRule rule_one = CreateGenericRule();
  rule_one.condition->url_filter = std::string("one.com");

  TestRule rule_two = CreateGenericRule();
  rule_two.condition->url_filter = std::string("two.com");

  enum RulesetMask {
    BLOCKER_ONE = 1<<0,
    BLOCKER_TWO = 1<<1,
  };

  RulesetManager* manager = ExtensionSystem::Get(browser_context())->info_map()->GetRulesetManager();
  ASSERT_TRUE(manager);

  std::unique_ptr<net::URLRequest> request_one = GetRequestForURL("http://one.com");
  std::unique_ptr<net::URLRequest> request_two = GetRequestForURL("http://two.com");
  std::unique_ptr<net::URLRequest> request_three = GetRequestForURL("http://three.com");

  // Test all possible combinations with |blocker_one| and |blocker_two|
  // enabled.
  const bool is_incognito_context = false;
  for (int mask = 0; mask < 4; mask++) {
    SCOPED_TRACE(base::StringPrintf("Testing ruleset mask %d", mask));

    ASSERT_EQ(0u, manager->GetMatcherCountForTest());
    std::string extension_id_one, extension_id_two;
    size_t expected_matcher_count = 0;

    // Add the required rulesets.
    if (mask & BLOCKER_ONE) {
      ++expected_matcher_count;
      std::unique_ptr<RulesetMatcher> matcher;
      ASSERT_NO_FATAL_FAILURE(CreateMatcherForRules({rule_one}, base::StringPrintf("%d-one", mask), &matcher));
      extension_id_one = last_loaded_extension()->id();
      manager->AddRuleset(extension_id_one, base::Time(), std::move(matcher));
    }
    if (mask & BLOCKER_TWO) {
      ++expected_matcher_count;
      std::unique_ptr<RulesetMatcher> matcher;
      ASSERT_NO_FATAL_FAILURE(CreateMatcherForRules({rule_two}, base::StringPrintf("%d-two", mask), &matcher));
      extension_id_two = last_loaded_extension()->id();
      manager->AddRuleset(extension_id_two, base::Time(), std::move(matcher));
    }

    ASSERT_EQ(expected_matcher_count, manager->GetMatcherCountForTest());

    EXPECT_EQ((mask & BLOCKER_ONE) != 0, manager->ShouldBlockRequest(is_incognito_context, *request_one));
    EXPECT_EQ((mask & BLOCKER_TWO) != 0, manager->ShouldBlockRequest(is_incognito_context, *request_two));
    EXPECT_FALSE(manager->ShouldBlockRequest(is_incognito_context, *request_three));

    // Remove the rulesets.
    manager->RemoveRuleset(extension_id_one);
    manager->RemoveRuleset(extension_id_two);
  }
}

TEST_P(RulesetManagerTest, IncognitoRequests) {
  InfoMap* info_map = ExtensionSystem::Get(browser_context())->info_map();
  RulesetManager* manager = info_map->GetRulesetManager();
  ASSERT_TRUE(manager);

  // Add an extension ruleset blocking "example.com".
  TestRule rule_one = CreateGenericRule();
  rule_one.condition->url_filter = std::string("example.com");
  std::unique_ptr<RulesetMatcher> matcher;
  ASSERT_NO_FATAL_FAILURE(CreateMatcherForRules({rule_one}, "test_extension", &matcher));
  manager->AddRuleset(last_loaded_extension()->id(), base::Time(), std::move(matcher));

  std::unique_ptr<net::URLRequest> request = GetRequestForURL("http://example.com");

  // By default, the extension is disabled in incognito mode. So requests from
  // incognito contexts should not be evaluated.
  // EXPECT_FALSE(util::IsIncognitoEnabled(last_loaded_extension()->id(), browser_context()));
  // EXPECT_FALSE(manager->ShouldBlockRequest(true /*is_incognito_context*/, *request));
  // EXPECT_TRUE(manager->ShouldBlockRequest(false /*is_incognito_context*/, *request));

  // Enabling the extension in incognito mode, should cause requests from
  // incognito contexts to also be evaluated.
  EXPECT_TRUE(manager->ShouldBlockRequest(true /*is_incognito_context*/, *request));
  // EXPECT_TRUE(manager->ShouldBlockRequest(false /*is_incognito_context*/, *request));
}

INSTANTIATE_TEST_CASE_P(,
                        RulesetManagerTest,
                        ::testing::Values(ExtensionLoadType::PACKED,
                                          ExtensionLoadType::UNPACKED));

}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
