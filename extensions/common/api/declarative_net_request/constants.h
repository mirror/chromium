// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_

namespace extensions {
namespace declarative_net_request {

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
