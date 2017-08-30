// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/parse_info.h"

#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "extensions/common/error_utils.h"

namespace extensions {
namespace declarative_net_request {

ParseInfo::ParseInfo(ParseResult result) : result_(result) {}
ParseInfo::ParseInfo(ParseResult result, size_t rule_index)
    : result_(result), rule_index_(rule_index) {}
ParseInfo::ParseInfo(const ParseInfo&) = default;
ParseInfo& ParseInfo::operator=(const ParseInfo&) = default;

std::string ParseInfo::GetErrorDescription(
    const base::StringPiece json_rules_filename) const {
  std::string error;
  switch (result_) {
    case ParseResult::SUCCESS:
      NOTREACHED();
      break;
    case ParseResult::ERROR_RESOURCE_TYPE_DUPLICATED:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(kErrorResourceTypeDuplicated,
                                             json_rules_filename,
                                             std::to_string(*rule_index_));
      break;
    case ParseResult::ERROR_EMPTY_REDIRECT_RULE_PRIORITY:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyRedirectRuleKey, json_rules_filename,
          std::to_string(*rule_index_), kPriorityKey);
      break;
    case ParseResult::ERROR_EMPTY_REDIRECT_URL:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyRedirectRuleKey, json_rules_filename,
          std::to_string(*rule_index_), kRedirectUrlKey);
      break;
    case ParseResult::ERROR_INVALID_RULE_ID:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(
          kErrorInvalidRuleKey, json_rules_filename,
          std::to_string(*rule_index_), kIDKey, std::to_string(kMinValidID));
      break;
    case ParseResult::ERROR_INVALID_REDIRECT_RULE_PRIORITY:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(
          kErrorInvalidRuleKey, json_rules_filename,
          std::to_string(*rule_index_), kPriorityKey,
          std::to_string(kMinValidPriority));
      break;
    case ParseResult::ERROR_NO_APPLICABLE_RESOURCE_TYPES:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(kErrorNoApplicableResourceTypes,
                                             json_rules_filename,
                                             std::to_string(*rule_index_));
      break;
    case ParseResult::ERROR_EMPTY_DOMAINS_LIST:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyList, json_rules_filename, std::to_string(*rule_index_),
          kDomainsKey);
      break;
    case ParseResult::ERROR_EMPTY_RESOURCE_TYPES_LIST:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyList, json_rules_filename, std::to_string(*rule_index_),
          kResourceTypesKey);
      break;
    case ParseResult::ERROR_EMPTY_URL_FILTER:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(
          kErrorEmptyUrlFilter, json_rules_filename,
          std::to_string(*rule_index_), kUrlFilterKey);
      break;
    case ParseResult::ERROR_INVALID_REDIRECT_URL:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(
          kErrorInvalidRedirectUrl, json_rules_filename,
          std::to_string(*rule_index_), kRedirectUrlKey);
      break;
    case ParseResult::ERROR_LIST_NOT_PASSED:
      DCHECK(!rule_index_);
      error = ErrorUtils::FormatErrorMessage(kErrorListNotPassed,
                                             json_rules_filename);
      break;
    case ParseResult::ERROR_DUPLICATE_IDS:
      DCHECK(rule_index_);
      error = ErrorUtils::FormatErrorMessage(kErrorDuplicateIDs,
                                             json_rules_filename,
                                             std::to_string(*rule_index_));
      break;
    case ParseResult::ERROR_PERSISTING_RULESET:
      DCHECK(!rule_index_);
      error =
          ErrorUtils::FormatErrorMessage(kErrorPersisting, json_rules_filename);
      break;
  }
  return error;
}

}  // namespace declarative_net_request
}  // namespace extensions
