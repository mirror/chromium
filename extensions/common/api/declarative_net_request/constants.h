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
  ERROR_RESOURCE_TYPE_DUPLICATED,
  ERROR_EMPTY_REDIRECT_RULE_PRIORITY,
  ERROR_EMPTY_REDIRECT_URL,
  ERROR_INVALID_RULE_ID,
  ERROR_INVALID_REDIRECT_RULE_PRIORITY,
  ERROR_RULE_NOT_MEANINGFUL,
  ERROR_EMPTY_DOMAINS_LIST,
  ERROR_EMPTY_RESOURCE_TYPES_LIST,
};

// Minimum valid value of a declarative rule ID.
constexpr int kMinValidID = 0;

// Minimum valid value of a declarative rule priority.
constexpr int kMinValidPriority = 0;

// Default priority used for rules where the priority is not explicity provided
// by an extension.
constexpr int kDefaultPriority = 0;

// Permission name.
constexpr char kAPIPermission[] = "declarativeNetRequest";

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_CONSTANTS_H_
