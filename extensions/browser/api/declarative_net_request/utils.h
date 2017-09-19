// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_UTILS_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_UTILS_H_

namespace base {
class Value;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {
class Extension;
struct InstallWarning;

namespace declarative_net_request {

// Indexes and persists |rules| for |extension|. In case of an error, returns
// false and populates |error|. On success, returns |ruleset_checksum|, which
// is a checksum of the persisted indexed ruleset file.
// Note: This must be called on a thread where file IO is allowed.
bool IndexAndPersistRules(const base::Value& rules,
                          const Extension& extension,
                          std::string* error,
                          std::vector<InstallWarning>* warnings,
                          int* ruleset_checksum);

// Returns true if |data| and |size| represent a valid data buffer corresponding
// to the indexed ruleset for |extension_id|.
bool IsValidRulesetData(const std::string& extension_id,
                        const uint8_t* data,
                        size_t size,
                        content::BrowserContext* context);

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_UTILS_H_
