// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/rules_manifest_info.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "components/version_info/channel.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"
#include "extensions/common/permissions/api_permission.h"

namespace extensions {

namespace keys = manifest_keys;
namespace errors = manifest_errors;

namespace declarative_net_request {

namespace {
const base::FilePath::CharType kJsonFileExtension[] =
    FILE_PATH_LITERAL(".json");
constexpr version_info::Channel kAPIChannel = version_info::Channel::UNKNOWN;

// Returns true if the Declarative Net Request API is available to the current
// environment.
bool IsAPIAvailable() {
  // TODO(karandeepb): Use Feature::IsAvailableToEnvironment() once it behaves
  // correctly.
  return kAPIChannel >= GetCurrentChannel();
}
}  // namespace

RulesManifestData::RulesManifestData(base::FilePath path)
    : json_ruleset_path(std::move(path)) {}
RulesManifestData::~RulesManifestData() = default;

// static
const base::FilePath* RulesManifestData::GetJSONRulesetPath(
    const Extension* extension) {
  Extension::ManifestData* data =
      extension->GetManifestData(keys::kDeclarativeNetRequestRulesetLocation);
  return data ? &(static_cast<RulesManifestData*>(data)->json_ruleset_path)
              : nullptr;
}

RulesManifestHandler::RulesManifestHandler() = default;
RulesManifestHandler::~RulesManifestHandler() = default;

bool RulesManifestHandler::Parse(Extension* extension, base::string16* error) {
  DCHECK(extension->manifest()->HasKey(
      keys::kDeclarativeNetRequestRulesetLocation));
  DCHECK(IsAPIAvailable());

  if (!PermissionsParser::HasAPIPermission(
          extension, APIPermission::kDeclarativeNetRequest)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kDeclarativeNetRequestPermissionNeeded, kAPIPermission,
        keys::kDeclarativeNetRequestRulesetLocation);
    return false;
  }

  std::string json_ruleset_location;
  if (!extension->manifest()->GetString(
          keys::kDeclarativeNetRequestRulesetLocation,
          &json_ruleset_location)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kRulesetLocationNotString,
        keys::kDeclarativeNetRequestRulesetLocation);
    return false;
  }

  base::FilePath path = base::FilePath::FromUTF8Unsafe(json_ruleset_location);
  if (path.ReferencesParent()) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kRulesetLocationRefersParent,
        keys::kDeclarativeNetRequestRulesetLocation);
    return false;
  }

  if (!path.MatchesExtension(kJsonFileExtension)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kRulesetLocationNotJSON,
        keys::kDeclarativeNetRequestRulesetLocation);
    return false;
  }

  if (path.IsAbsolute()) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kRulesetLocationIsAbsolutePath,
        keys::kDeclarativeNetRequestRulesetLocation);
    return false;
  }

  path = extension->path().Append(path);

  extension->SetManifestData(
      keys::kDeclarativeNetRequestRulesetLocation,
      base::MakeUnique<RulesManifestData>(std::move(path)));

  return true;
}

bool RulesManifestHandler::Validate(
    const Extension* extension,
    std::string* error,
    std::vector<InstallWarning>* warnings) const {
  DCHECK(IsAPIAvailable());

  const base::FilePath* json_ruleset_path =
      RulesManifestData::GetJSONRulesetPath(extension);
  if (base::PathExists(*json_ruleset_path))
    return true;
  *error = ErrorUtils::FormatErrorMessage(
      errors::kRulesetLocationIsInvalidPath,
      keys::kDeclarativeNetRequestRulesetLocation);
  return false;
}

const std::vector<std::string> RulesManifestHandler::Keys() const {
  return ManifestHandler::SingleKey(
      keys::kDeclarativeNetRequestRulesetLocation);
}

}  // namespace declarative_net_request
}  // namespace extensions
