// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_

namespace extensions {
namespace declarative_net_request {

// The result of parsing JSON rules provided by an extension.
enum class ParseResult {
  SUCCESS,
  ERROR_LIST_NOT_PASSED,
  ERROR_DUPLICATE_IDS,
  ERROR_RESOURCE_TYPE_DUPLICATED,
  ERROR_PERSISTING_RULESET,
  ERROR_EMPTY_REDIRECT_RULE_PRIORITY,
  ERROR_EMPTY_REDIRECT_URL,
  ERROR_INVALID_RULE_ID,
  ERROR_INVALID_REDIRECT_RULE_PRIORITY,
  ERROR_JSON_PARSE,
  ERROR_RULE_NOT_MEANINGFUL,
};

// Minimum valid value of a declarative rule ID.
constexpr int kMinValidID = 0;

// Minimum valid value of a declarative rule priority.
constexpr int kMinValidPriority = 0;

// Default priority used for rules where the priority is not explicity provided
// by an extension.
constexpr int kDefaultPriority = 0;

// JSON keys for the API.
constexpr char kIDKey[] = "id";
constexpr char kPriorityKey[] = "priority";
constexpr char kConditionKey[] = "condition";
constexpr char kActionKey[] = "action";

constexpr char kUrlFilterKey[] = "urlFilter";
constexpr char kUrlFilterIsCaseSensitiveKey[] = "urlFilterIsCaseSensitive";
constexpr char kDomainsKey[] = "domains";
constexpr char kExcludedDomainsKey[] = "excludedDomains";
constexpr char kResourceTypesKey[] = "resourceTypes";
constexpr char kExcludedResourceTypesKey[] = "excludedResourceTypes";
constexpr char kDomainTypeKey[] = "domainsType";
constexpr char kActionTypeKey[] = "type";
constexpr char kRedirectUrlKey[] = "redirectUrl";

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
