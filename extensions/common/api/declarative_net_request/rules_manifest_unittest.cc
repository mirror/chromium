// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/path_service.h"
#include "base/strings/pattern.h"
#include "components/version_info/version_info.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request/rules_manifest_info.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_paths.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace keys = manifest_keys;
namespace errors = manifest_errors;

namespace declarative_net_request {
namespace {

// Helper to convert a vector of strings to a ListValue.
std::unique_ptr<base::ListValue> GetListValue(
    const std::vector<std::string>& vec) {
  ListBuilder builder;
  for (const std::string& str : vec)
    builder.Append(str);
  return builder.Build();
}

// Helper to build an extension manifest which uses the
// kDeclarativeNetRequestRulesetLocation manifest key.
std::unique_ptr<base::DictionaryValue> GetGenericManifest() {
  return DictionaryBuilder()
      .Set(keys::kName, "Test extension")
      .Set(keys::kDeclarativeNetRequestRulesetLocation, "rules_file.json")
      .Set(keys::kPermissions,
           ListBuilder().Append("declarativeNetRequest").Build())
      .Set(keys::kVersion, "1.0")
      .Set(keys::kManifestVersion, 2)
      .Build();
}

// Helper structs to simplify building base::Values which are later used to
// serialize to JSON.
struct TestRuleCondition {
  base::Optional<std::string> url_filter;
  base::Optional<bool> url_filter_is_case_sensitive;
  base::Optional<std::vector<std::string>> domains;
  base::Optional<std::vector<std::string>> excluded_domains;
  base::Optional<std::vector<std::string>> resource_types;
  base::Optional<std::vector<std::string>> excluded_resource_types;
  base::Optional<std::string> domain_type;
  std::unique_ptr<base::DictionaryValue> ToValue() const {
    std::unique_ptr<base::DictionaryValue> value =
        base::MakeUnique<base::DictionaryValue>();
    if (url_filter)
      value->SetString(kUrlFilterKey, *url_filter);
    if (url_filter_is_case_sensitive) {
      value->SetBoolean(kUrlFilterIsCaseSensitiveKey,
                        *url_filter_is_case_sensitive);
    }
    if (domains)
      value->Set(kDomainsKey, GetListValue(*domains));
    if (excluded_domains)
      value->Set(kExcludedDomainsKey, GetListValue(*excluded_domains));
    if (resource_types)
      value->Set(kResourceTypesKey, GetListValue(*resource_types));
    if (excluded_resource_types)
      value->Set(kExcludedResourceTypesKey,
                 GetListValue(*excluded_resource_types));
    if (domain_type)
      value->SetString(kDomainTypeKey, *domain_type);
    return value;
  }
};

struct TestRuleAction {
  base::Optional<std::string> type;
  base::Optional<std::string> redirect_url;
  std::unique_ptr<base::DictionaryValue> ToValue() const {
    std::unique_ptr<base::DictionaryValue> value =
        base::MakeUnique<base::DictionaryValue>();
    if (type)
      value->SetString(kActionTypeKey, *type);
    if (redirect_url)
      value->SetString(kRedirectUrlKey, *redirect_url);
    return value;
  }
};

struct TestRule {
  base::Optional<int> id;
  base::Optional<int> priority;
  base::Optional<TestRuleCondition> condition;
  base::Optional<TestRuleAction> action;
  std::unique_ptr<base::DictionaryValue> ToValue() const {
    std::unique_ptr<base::DictionaryValue> value =
        base::MakeUnique<base::DictionaryValue>();
    if (id)
      value->SetInteger(kIDKey, *id);
    if (priority)
      value->SetInteger(kPriorityKey, *priority);
    if (condition)
      value->Set(kConditionKey, condition->ToValue());
    if (action)
      value->Set(kActionKey, action->ToValue());
    return value;
  }
};

// Test fixture allowing clients to load extensions with declarative rules.
// TODO(crbug.com/696822): Move to extensions/browser and test packed extensions
// as well.
class RulesManifestTest : public testing::Test {
 public:
  RulesManifestTest() : channel_(::version_info::Channel::UNKNOWN) {}

 protected:
  // Overrides the default rules file path.
  void SetRulesFilePath(base::FilePath path) {
    rules_file_path_ = std::move(path);
  }

  // Overrides the default manifest.
  void SetManifest(std::unique_ptr<base::Value> manifest) {
    manifest_ = std::move(manifest);
  }

  // Loads the extension and verifies that an error matching |pattern| is
  // observed.
  void LoadAndExpectError(const base::StringPiece pattern) {
    Finish();
    std::string error;
    scoped_refptr<Extension> extension = file_util::LoadExtension(
        temp_dir_.GetPath(), Manifest::UNPACKED, Extension::NO_FLAGS, &error);
    EXPECT_FALSE(extension);
    EXPECT_TRUE(base::MatchPattern(error, pattern));
  }

  // Loads the extension and verifies that the json ruleset is successfully
  // indexed.
  void LoadAndExpectSuccess() {
    Finish();
    std::string error;
    scoped_refptr<Extension> extension = file_util::LoadExtension(
        temp_dir_.GetPath(), Manifest::UNPACKED, Extension::NO_FLAGS, &error);
    EXPECT_TRUE(extension);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(temp_dir_.GetPath().Append(rules_file_path_),
              *RulesManifestData::GetJSONRulesetPath(extension.get()));
  }

 private:
  // Helper to persist the JSON ruleset and manifest file.
  void Finish() {
    DCHECK(!finished_);
    finished_ = true;

    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());

    base::FilePath rules_path = temp_dir_.GetPath().Append(rules_file_path_);

    // Create parent directory of |rules_path| if it doesn't exist.
    EXPECT_TRUE(base::CreateDirectory(rules_path.DirName()));

    // Persist an empty JSON rules file.
    JSONFileValueSerializer(rules_path).Serialize(base::Value());

    // Persist manifest file.
    JSONFileValueSerializer(temp_dir_.GetPath().Append(kManifestFilename))
        .Serialize(*manifest_);
  }

  bool finished_ = false;
  std::unique_ptr<base::Value> manifest_ = GetGenericManifest();
  base::FilePath rules_file_path_ =
      base::FilePath(FILE_PATH_LITERAL("rules_file.json"));
  base::ScopedTempDir temp_dir_;
  ScopedCurrentChannel channel_;

  DISALLOW_COPY_AND_ASSIGN(RulesManifestTest);
};

TEST_F(RulesManifestTest, InvalidRulesetLocationType) {
  std::unique_ptr<base::DictionaryValue> manifest = GetGenericManifest();
  manifest->SetInteger(keys::kDeclarativeNetRequestRulesetLocation, 3);
  SetManifest(std::move(manifest));
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kRulesetLocationNotString,
      keys::kDeclarativeNetRequestRulesetLocation));
}

TEST_F(RulesManifestTest, RulesetLocationRefersParent) {
  std::unique_ptr<base::DictionaryValue> manifest = GetGenericManifest();
  manifest->SetString(keys::kDeclarativeNetRequestRulesetLocation,
                      "../rules_file.json");
  SetManifest(std::move(manifest));
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kRulesetLocationRefersParent,
      keys::kDeclarativeNetRequestRulesetLocation));
}

TEST_F(RulesManifestTest, RulesetLocationNotJSON) {
  std::unique_ptr<base::DictionaryValue> manifest = GetGenericManifest();
  manifest->SetString(keys::kDeclarativeNetRequestRulesetLocation,
                      "rules_file.exe");
  SetManifest(std::move(manifest));
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kRulesetLocationNotJSON,
      keys::kDeclarativeNetRequestRulesetLocation));
}

TEST_F(RulesManifestTest, RulesetLocationNotRelative) {
  std::unique_ptr<base::DictionaryValue> manifest = GetGenericManifest();

  std::string absolute_path;
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
  absolute_path = "C://Desktop/rules_file.json";
#else
  absolute_path = "/Users/Desktop/rules_file.json";
#endif
  manifest->SetString(keys::kDeclarativeNetRequestRulesetLocation,
                      absolute_path);

  SetManifest(std::move(manifest));
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kRulesetLocationIsAbsolutePath,
      keys::kDeclarativeNetRequestRulesetLocation));
}

TEST_F(RulesManifestTest, NonExistentRulesFile) {
  std::unique_ptr<base::DictionaryValue> manifest = GetGenericManifest();
  manifest->SetString(keys::kDeclarativeNetRequestRulesetLocation,
                      "invalid_rules_file.json");
  SetManifest(std::move(manifest));
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kRulesetLocationIsInvalidPath,
      keys::kDeclarativeNetRequestRulesetLocation));
}

TEST_F(RulesManifestTest, NeedsDeclarativeNetRequestPermission) {
  std::unique_ptr<base::DictionaryValue> manifest = GetGenericManifest();
  // Remove "declarativeNetRequest" permission.
  manifest->Remove(keys::kPermissions, nullptr);
  SetManifest(std::move(manifest));
  LoadAndExpectError(errors::kDeclarativeNetRequestPermissionNeeded);
}

TEST_F(RulesManifestTest, RulesFileInNestedDirectory) {
  std::unique_ptr<base::DictionaryValue> manifest = GetGenericManifest();
  SetRulesFilePath(base::FilePath(FILE_PATH_LITERAL("dir"))
                       .Append(FILE_PATH_LITERAL("rules_file.json")));
  manifest->SetString(keys::kDeclarativeNetRequestRulesetLocation,
                      "dir/rules_file.json");
  SetManifest(std::move(manifest));
  LoadAndExpectSuccess();
}

}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
