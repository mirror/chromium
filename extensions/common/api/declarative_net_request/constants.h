// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_

namespace extensions {
namespace declarative_net_request {

// Permission name.
constexpr char kAPIPermission[] = "declarativeNetRequest";

// Minimum valid value of a declarative rule ID.
constexpr int kMinValidID = 1;

// Minimum valid value of a declarative rule priority.
constexpr int kMinValidPriority = 1;

// Default priority used for rules when the priority is not explicity provided
// by an extension.
constexpr int kDefaultPriority = 1;

// Keys used in rules.
constexpr char kIDKey[] = "id";
constexpr char kPriorityKey[] = "priority";
constexpr char kRuleConditionKey[] = "condition";
constexpr char kRuleActionKey[] = "action";
constexpr char kUrlFilterKey[] = "urlFilter";
constexpr char kIsUrlFilterCaseSensitiveKey[] = "isUrlFilterCaseSensitive";
constexpr char kDomainsKey[] = "domains";
constexpr char kExcludedDomainsKey[] = "excludedDomains";
constexpr char kResourceTypesKey[] = "resourceTypes";
constexpr char kExcludedResourceTypesKey[] = "excludedResourceTypes";
constexpr char kDomainTypeKey[] = "domainType";
constexpr char kRuleActionTypeKey[] = "type";
constexpr char kRedirectUrlKey[] = "redirectUrl";

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
