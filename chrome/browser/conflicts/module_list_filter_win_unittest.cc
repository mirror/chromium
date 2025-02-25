// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/module_list_filter_win.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/i18n/case_conversion.h"
#include "base/sha1.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/conflicts/module_info_win.h"
#include "chrome/browser/conflicts/proto/module_list.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Typedef for convenience.
using ModuleInfo = std::pair<ModuleInfoKey, ModuleInfoData>;

// Writes the |contents| to |file_path|. Returns true on success.
bool WriteStringToFile(const base::FilePath& file_path,
                       const std::string& contents) {
  int result = base::WriteFile(file_path, contents.data(),
                               static_cast<int>(contents.size()));
  return contents.size() == static_cast<size_t>(result);
}

// Helper class to build and serialize a ModuleList.
class ModuleListBuilder {
 public:
  explicit ModuleListBuilder(const base::FilePath& module_list_path)
      : module_list_path_(module_list_path) {
    // Include an empty blacklist and whitelist.
    module_list_.mutable_blacklist();
    module_list_.mutable_whitelist();
  }

  // Adds a module to the whitelist.
  void AddWhitelistedModule(const ModuleInfoKey& module_key,
                            const ModuleInfoData& module_data) {
    chrome::conflicts::ModuleGroup* module_group =
        module_list_.mutable_whitelist()->add_module_groups();

    chrome::conflicts::Module* module = module_group->add_modules();

    module->set_basename_hash(base::SHA1HashString(base::UTF16ToUTF8(
        base::i18n::ToLower(module_data.inspection_result->basename))));

    std::string code_id = base::StringPrintf(
        "%08X%x", module_key.module_time_date_stamp, module_key.module_size);
    module->set_code_id_hash(base::SHA1HashString(code_id));
  }

  // Adds a module to the blacklist.
  void AddBlacklistedModule(
      const ModuleInfoKey& module_key,
      const ModuleInfoData& module_data,
      bool allow_load_value,
      chrome::conflicts::BlacklistMessageType message_type,
      const std::string& message_url) {
    chrome::conflicts::BlacklistModuleGroup* module_group =
        module_list_.mutable_blacklist()->add_module_groups();

    chrome::conflicts::BlacklistAction* blacklist_action =
        module_group->mutable_action();
    blacklist_action->set_allow_load(true);
    blacklist_action->set_message_type(message_type);
    blacklist_action->set_message_url(message_url);

    chrome::conflicts::Module* module =
        module_group->mutable_modules()->add_modules();

    module->set_basename_hash(base::SHA1HashString(base::UTF16ToUTF8(
        base::i18n::ToLower(module_data.inspection_result->basename))));

    std::string code_id = base::StringPrintf(
        "%08X%x", module_key.module_time_date_stamp, module_key.module_size);
    module->set_code_id_hash(base::SHA1HashString(code_id));
  }

  // Serializes the |module_list_| to |module_list_path_|. Returns true on
  // success.
  bool Finalize() {
    std::string contents;
    return module_list_.SerializeToString(&contents) &&
           WriteStringToFile(module_list_path_, contents);
  }

 private:
  const base::FilePath module_list_path_;

  chrome::conflicts::ModuleList module_list_;

  DISALLOW_COPY_AND_ASSIGN(ModuleListBuilder);
};

// Creates a pair of ModuleInfoKey and ModuleInfoData with the necessary
// information to call in IsModuleWhitelisted().
ModuleInfo CreateModuleInfo(const base::FilePath& module_path,
                            uint32_t module_size,
                            uint32_t module_time_date_stamp) {
  ModuleInfo result(std::piecewise_construct,
                    std::forward_as_tuple(module_path, module_size,
                                          module_time_date_stamp, 0),
                    std::forward_as_tuple());

  result.second.inspection_result = std::make_unique<ModuleInspectionResult>();

  result.second.inspection_result->basename = module_path.BaseName().value();

  return result;
}

constexpr wchar_t kDllPath1[] = L"c:\\path\\to\\module.dll";
constexpr wchar_t kDllPath2[] = L"c:\\some\\shellextension.dll";

}  // namespace

class ModuleListFilterTest : public ::testing::Test {
 public:
  ModuleListFilterTest() : dll1_(kDllPath1), dll2_(kDllPath2) {}

  ~ModuleListFilterTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(scoped_temp_dir_.CreateUniqueTempDir());
    module_list_path_ = scoped_temp_dir_.GetPath().Append(L"ModuleList.bin");
  }

  const base::FilePath& module_list_path() { return module_list_path_; }
  ModuleListFilter& module_list_filter() { return module_list_filter_; }

  const base::FilePath dll1_;
  const base::FilePath dll2_;

 private:
  base::ScopedTempDir scoped_temp_dir_;

  base::FilePath module_list_path_;

  ModuleListFilter module_list_filter_;

  DISALLOW_COPY_AND_ASSIGN(ModuleListFilterTest);
};

TEST_F(ModuleListFilterTest, WhitelistedModules) {
  ModuleInfo module_1 = CreateModuleInfo(dll1_, 0123, 4567);
  ModuleInfo module_2 = CreateModuleInfo(dll2_, 7654, 3210);

  ModuleListBuilder module_list_builder(module_list_path());
  module_list_builder.AddWhitelistedModule(module_1.first, module_1.second);
  ASSERT_TRUE(module_list_builder.Finalize());

  ASSERT_TRUE(module_list_filter().Initialize(module_list_path()));

  EXPECT_TRUE(
      module_list_filter().IsWhitelisted(module_1.first, module_1.second));
  EXPECT_FALSE(
      module_list_filter().IsWhitelisted(module_2.first, module_2.second));
}

TEST_F(ModuleListFilterTest, BlacklistedModules) {
  const char kFurtherInfoURL[] = "http://www.further-info.com";

  ModuleInfo module_1 = CreateModuleInfo(dll1_, 0123, 4567);
  ModuleInfo module_2 = CreateModuleInfo(dll2_, 7654, 3210);

  ModuleListBuilder module_list_builder(module_list_path());
  module_list_builder.AddBlacklistedModule(
      module_1.first, module_1.second, true,
      chrome::conflicts::BlacklistMessageType::FURTHER_INFORMATION,
      kFurtherInfoURL);
  ASSERT_TRUE(module_list_builder.Finalize());

  ASSERT_TRUE(module_list_filter().Initialize(module_list_path()));

  std::unique_ptr<chrome::conflicts::BlacklistAction> blacklist_action =
      module_list_filter().IsBlacklisted(module_1.first, module_1.second);
  ASSERT_TRUE(blacklist_action);
  EXPECT_TRUE(blacklist_action->allow_load());
  EXPECT_EQ(chrome::conflicts::BlacklistMessageType::FURTHER_INFORMATION,
            blacklist_action->message_type());
  EXPECT_EQ(kFurtherInfoURL, blacklist_action->message_url());
  EXPECT_FALSE(
      module_list_filter().IsBlacklisted(module_2.first, module_2.second));
}
