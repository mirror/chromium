// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_

#include "extensions/common/api/declarative_net_request/constants.h"

namespace extensions {
namespace declarative_net_request {

// The result of parsing JSON rules provided by an extension. Can correspond to
// a single or multiple rules.
enum class ParseResult {
  SUCCESS,
  ERROR_RESOURCE_TYPE_DUPLICATED,
  ERROR_EMPTY_REDIRECT_RULE_PRIORITY,
  ERROR_EMPTY_REDIRECT_URL,
  ERROR_INVALID_RULE_ID,
  ERROR_INVALID_REDIRECT_RULE_PRIORITY,
  ERROR_NO_APPLICABLE_RESOURCE_TYPES,
  ERROR_EMPTY_DOMAINS_LIST,
  ERROR_EMPTY_RESOURCE_TYPES_LIST,
  ERROR_EMPTY_URL_FILTER,
  ERROR_INVALID_REDIRECT_URL,
  ERROR_LIST_NOT_PASSED,
  ERROR_DUPLICATE_IDS,
  ERROR_PERSISTING_RULESET,
};

// Rule parsing errors.
constexpr char kErrorResourceTypeDuplicated[] =
    "*: Rule at index * includes and excludes the same resource.";
constexpr char kErrorEmptyRedirectRuleKey[] =
    "*: Rule at index * does not specify the value for * key. This is required "
    "for redirect rules.";
constexpr char kErrorInvalidRuleKey[] =
    "*: Rule at index * has an invalid value for * key. This should be greater "
    "than or equal to *.";
constexpr char kErrorNoApplicableResourceTypes[] =
    "*: Rule at index * is not applicable to any resource type.";
constexpr char kErrorEmptyList[] =
    "*: Rule at index * cannot have an empty list as the value for * key.";
constexpr char kErrorEmptyUrlFilter[] =
    "*: Rule at index * cannot have an empty value for * key.";
constexpr char kErrorInvalidRedirectUrl[] =
    "*: Rule at index * does not provide a valid URL for * key.";
constexpr char kErrorListNotPassed[] =
    "*: Rules file does not have a valid list.";
constexpr char kErrorDuplicateIDs[] =
    "*: Rule at index * does not have a unique ID.";
// Don't surface the actual error to the user, since it's an implementation
// detail.
constexpr char kErrorPersisting[] = "*: Rules file could not be parsed.";

// Rule parsing install warnings.
constexpr char kRulesNotParsedWarning[] =
    "Declarative Net Request: Not all rules were successfully parsed.";

// Histogram names.
constexpr char kIndexAndPersistRulesDurationHistogram[] =
    "Extensions.DeclarativeNetRequest.IndexAndPersistRulesDuration";
constexpr char kIndexRulesDurationHistogram[] =
    "Extensions.DeclarativeNetRequest.IndexRulesDuration";
constexpr char kManifestRulesCountHistogram[] =
    "Extensions.DeclarativeNetRequest.ManifestRulesCount";

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
