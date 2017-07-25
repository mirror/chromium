// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/rules_manifest_info.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request/utils.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"
#include "extensions/common/permissions/api_permission.h"

namespace extensions {

namespace keys = manifest_keys;
namespace errors = manifest_errors;

namespace declarative_net_request {

RulesManifestData::RulesManifestData(const ExtensionResource& resource)
    : resource(resource) {}
RulesManifestData::~RulesManifestData() = default;

// static
const ExtensionResource* RulesManifestData::GetRulesetResource(
    const Extension* extension) {
  Extension::ManifestData* data =
      extension->GetManifestData(keys::kDeclarativeNetRequestKey);
  if (!data)
    return nullptr;
  return &(static_cast<RulesManifestData*>(data)->resource);
}

RulesManifestHandler::RulesManifestHandler() = default;
RulesManifestHandler::~RulesManifestHandler() = default;

bool RulesManifestHandler::Parse(Extension* extension, base::string16* error) {
  DCHECK(extension->manifest()->HasKey(keys::kDeclarativeNetRequestKey));
  DCHECK(IsAPIAvailable());

  if (!PermissionsParser::HasAPIPermission(
          extension, APIPermission::kDeclarativeNetRequest)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kDeclarativeNetRequestPermissionNeeded, kAPIPermission,
        keys::kDeclarativeNetRequestKey);
    return false;
  }

  const base::DictionaryValue* dict = nullptr;
  if (!extension->manifest()->GetDictionary(keys::kDeclarativeNetRequestKey,
                                            &dict)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidDeclarativeNetRequestKey,
        keys::kDeclarativeNetRequestKey);
    return false;
  }

  std::string json_ruleset_location;
  if (!dict->GetString(keys::kDeclarativeRulesFileKey,
                       &json_ruleset_location)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidDeclarativeRulesFileKey,
        keys::kDeclarativeNetRequestKey, keys::kDeclarativeRulesFileKey);
    return false;
  }

  extension->SetManifestData(
      keys::kDeclarativeNetRequestKey,
      base::MakeUnique<RulesManifestData>(
          extension->GetResource(json_ruleset_location)));
  return true;
}

bool RulesManifestHandler::Validate(
    const Extension* extension,
    std::string* error,
    std::vector<InstallWarning>* warnings) const {
  DCHECK(IsAPIAvailable());

  const ExtensionResource* resource =
      RulesManifestData::GetRulesetResource(extension);
  DCHECK(resource);

  // The file path is valid.
  if (!resource->GetFilePath().empty())
    return true;

  *error = ErrorUtils::FormatErrorMessage(errors::kRulesFileIsInvalid,
                                          keys::kDeclarativeNetRequestKey,
                                          keys::kDeclarativeRulesFileKey);
  return false;
}

const std::vector<std::string> RulesManifestHandler::Keys() const {
  return ManifestHandler::SingleKey(keys::kDeclarativeNetRequestKey);
}

}  // namespace declarative_net_request
}  // namespace extensions
