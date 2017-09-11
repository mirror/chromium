// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/module_list_win.h"

#include <string>
#include <utility>
#include <vector>

#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/sha1.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/version.h"
#include "chrome/browser/conflicts/module_info_win.h"
#include "chrome/browser/conflicts/proto/module_list.pb.h"

namespace {

std::string GenerateCodeId(const ModuleInfoKey& module_key) {
  return base::StringPrintf("%08X%x", module_key.module_time_date_stamp,
                            module_key.module_size);
}

base::Version ConvertVersion(
    const chrome::conflicts::ModuleVersion& module_version) {
  std::vector<uint32_t> components(1);
  components.push_back(module_version.major_version());
  if (module_version.has_minor_version()) {
    components.push_back(module_version.minor_version());
    if (module_version.has_patch_version()) {
      components.push_back(module_version.patch_version());
      if (module_version.has_revision_version()) {
        components.push_back(module_version.revision_version());
      }
    }
  }
  return base::Version(components);
}

base::Version ConvertVersion(const base::string16& version) {
  auto str_components = base::SplitStringPiece(
      version, L".", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  std::vector<uint32_t> components;
  components.reserve(str_components.size());
  for (const auto& str_component : str_components) {
    uint32_t value = 0;
    CHECK(base::StringToUint(str_component, &value));
    components.push_back(value);
  }

  return base::Version(components);
}

bool MatchesModuleGroup(const chrome::conflicts::ModuleGroup& module_group,
                        const ModuleInfoKey& module_key,
                        const ModuleInfoData& module_data) {
  // Publisher?
  if (module_group.has_publisher_hash() &&
      module_group.publisher_hash() !=
          base::SHA1HashString(base::UTF16ToUTF8(
              module_data.inspection_result->certificate_info.subject))) {
    return false;
  }

  // Directory?
  if (module_group.has_directory_hash() &&
      module_group.directory_hash() !=
          base::SHA1HashString(base::UTF16ToUTF8(
              base::i18n::ToLower(module_key.module_path.value())))) {
    return false;
  }

  // Now look at each module in the group in detail.
  for (const auto& module : module_group.modules()) {
    // Basename
    if (module.has_basename_hash() &&
        module.basename_hash() !=
            base::SHA1HashString(base::UTF16ToUTF8(base::i18n::ToLower(
                module_data.inspection_result->basename)))) {
      continue;
    }

    // Code id
    if (module.has_code_id_hash() &&
        module.code_id_hash() !=
            base::SHA1HashString(GenerateCodeId(module_key))) {
      continue;
    }

    // Min version
    if (module.has_version_min() &&
        ConvertVersion(module.version_min()) >
            ConvertVersion(module_data.inspection_result->version)) {
      continue;
    }

    // Max version
    if (module.has_version_max() &&
        ConvertVersion(module.version_max()) <
            ConvertVersion(module_data.inspection_result->version)) {
      continue;
    }

    return true;
  }

  return false;
}

}  // namespace

ModuleList::ModuleList(
    std::unique_ptr<chrome::conflicts::ModuleList> module_list)
    : module_list_(std::move(module_list)) {
  DCHECK(module_list_);
}

ModuleList::~ModuleList() = default;

bool ModuleList::IsBlacklisted(const ModuleInfoKey& module_key,
                               const ModuleInfoData& module_data) {
  // Check each module groups.
  for (const auto& blacklist_module_group :
       module_list_->blacklist().module_groups()) {
    // Whats the action?
    if (blacklist_module_group.action().allow_load())
      continue;

    // Look at the modules in the group.
    const auto& module_group = blacklist_module_group.modules();

    if (MatchesModuleGroup(module_group, module_key, module_data))
      return true;
  }
  return false;
}

bool ModuleList::IsWhitelisted(const ModuleInfoKey& module_key,
                               const ModuleInfoData& module_data) {
  for (const auto& module_group : module_list_->whitelist().module_groups()) {
    if (MatchesModuleGroup(module_group, module_key, module_data))
      return true;
  }

  return false;
}

bool ModuleList::ShouldShowWarning(const ModuleInfoKey& module_key,
                                   const ModuleInfoData& module_data) {
  // Check each module groups.
  for (const auto& blacklist_module_group :
       module_list_->blacklist().module_groups()) {
    // Whats the action?
    if (!blacklist_module_group.action().allow_load())
      continue;

    // Look at the modules in the group.
    const auto& module_group = blacklist_module_group.modules();

    // TODO(pmonette): Also return the message type and text.
    if (MatchesModuleGroup(module_group, module_key, module_data))
      return true;
  }
  return false;
}
