// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/module_list_filter_win.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/case_conversion.h"
#include "base/sha1.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/conflicts/module_info_win.h"

namespace {

std::string GenerateCodeId(const ModuleInfoKey& module_key) {
  return base::StringPrintf("%08X%x", module_key.module_time_date_stamp,
                            module_key.module_size);
}

bool MatchesModuleGroup(const chrome::conflicts::ModuleGroup& module_group,
                        const std::string& module_basename_hash,
                        const std::string& module_code_id_hash) {
  // While the ModuleList proto supports fine-grained filtering using the
  // publisher name and the module directory, this capability is not used yet.

  // Now look at each module in the group in detail.
  for (const auto& module : module_group.modules()) {
    // A valid entry contains both the basename and the code id.
    if (!module.has_basename_hash() || !module.has_code_id_hash())
      continue;

    // Match against the module.
    if (module.basename_hash() == module_basename_hash &&
        module.code_id_hash() == module_code_id_hash) {
      return true;
    }
  }

  return false;
}

}  // namespace

ModuleListFilter::ModuleListFilter() = default;

ModuleListFilter::~ModuleListFilter() = default;

bool ModuleListFilter::Initialize(const base::FilePath& module_list_path) {
  DCHECK(!initialized_);

  std::string contents;
  initialized_ = base::ReadFileToString(module_list_path, &contents) &&
                 module_list_.ParseFromString(contents);

  return initialized_;
}

bool ModuleListFilter::IsWhitelisted(const ModuleInfoKey& module_key,
                                     const ModuleInfoData& module_data) const {
  DCHECK(initialized_);

  // Precompute the hash of the basename and of the code id.
  const std::string module_basename_hash =
      base::SHA1HashString(base::UTF16ToUTF8(
          base::i18n::ToLower(module_data.inspection_result->basename)));
  const std::string module_code_id_hash =
      base::SHA1HashString(GenerateCodeId(module_key));

  for (const auto& module_group : module_list_.whitelist().module_groups()) {
    if (MatchesModuleGroup(module_group, module_basename_hash,
                           module_code_id_hash))
      return true;
  }

  return false;
}

std::unique_ptr<chrome::conflicts::BlacklistAction>
ModuleListFilter::IsBlacklisted(const ModuleInfoKey& module_key,
                                const ModuleInfoData& module_data) const {
  DCHECK(initialized_);

  // Precompute the hash of the basename and of the code id.
  const std::string module_basename_hash =
      base::SHA1HashString(base::UTF16ToUTF8(
          base::i18n::ToLower(module_data.inspection_result->basename)));
  const std::string module_code_id_hash =
      base::SHA1HashString(GenerateCodeId(module_key));

  for (const auto& blacklist_module_group :
       module_list_.blacklist().module_groups()) {
    if (MatchesModuleGroup(blacklist_module_group.modules(),
                           module_basename_hash, module_code_id_hash)) {
      return std::make_unique<chrome::conflicts::BlacklistAction>(
          blacklist_module_group.action());
    }
  }

  return nullptr;
}
