// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/strings/pattern.h"
#include "components/version_info/version_info.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request/rules_manifest_info.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
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

// Helper to build an extension manifest which uses the
// kDeclarativeNetRequestRulesetLocation manifest key.
std::unique_ptr<base::DictionaryValue> GetGenericManifest() {
  return DictionaryBuilder()
      .Set(keys::kName, "Test extension")
      .Set(keys::kDeclarativeNetRequestRulesetLocation, "rules_file.json")
      .Set(keys::kPermissions, ListBuilder().Append(kAPIPermission).Build())
      .Set(keys::kVersion, "1.0")
      .Set(keys::kManifestVersion, 2)
      .Build();
}

// Fixture testing the kDeclarativeNetRequestRulesetLocation manifest key.
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

  // Loads the extension and verifies that the JSON ruleset location is
  // correctly set up.
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
  // Helper to persist the manifest file and an empty ruleset.
  void Finish() {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());

    base::FilePath rules_path = temp_dir_.GetPath().Append(rules_file_path_);

    // Create parent directory of |rules_path| if it doesn't exist.
    EXPECT_TRUE(base::CreateDirectory(rules_path.DirName()));

    // Persist an empty ruleset file.
    EXPECT_EQ(0, base::WriteFile(rules_path, nullptr /*data*/, 0 /*size*/));

    // Persist manifest file.
    JSONFileValueSerializer(temp_dir_.GetPath().Append(kManifestFilename))
        .Serialize(*manifest_);
  }

  std::unique_ptr<base::Value> manifest_ = GetGenericManifest();
  base::FilePath rules_file_path_ =
      base::FilePath(FILE_PATH_LITERAL("rules_file.json"));
  base::ScopedTempDir temp_dir_;
  ScopedCurrentChannel channel_;

  DISALLOW_COPY_AND_ASSIGN(RulesManifestTest);
};

TEST_F(RulesManifestTest, EmptyRuleset) {
  LoadAndExpectSuccess();
}

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

  base::FilePath absolute_path;
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
  absolute_path =
      base::FilePath(FILE_PATH_LITERAL("C:/Desktop/rules_file.json"));
#else
  absolute_path =
      base::FilePath(FILE_PATH_LITERAL("/Users/Desktop/rules_file.json"));
#endif
  ASSERT_TRUE(absolute_path.IsAbsolute());
  manifest->SetString(keys::kDeclarativeNetRequestRulesetLocation,
                      absolute_path.AsUTF8Unsafe());

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
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kDeclarativeNetRequestPermissionNeeded, kAPIPermission,
      keys::kDeclarativeNetRequestRulesetLocation));
}

TEST_F(RulesManifestTest, RulesFileInNestedDirectory) {
  std::unique_ptr<base::DictionaryValue> manifest = GetGenericManifest();
  base::FilePath nested_path =
      base::FilePath(FILE_PATH_LITERAL("dir"))
          .Append(FILE_PATH_LITERAL("rules_file.json"));
  SetRulesFilePath(nested_path);
  manifest->SetString(keys::kDeclarativeNetRequestRulesetLocation,
                      nested_path.AsUTF8Unsafe());
  SetManifest(std::move(manifest));
  LoadAndExpectSuccess();
}

}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
