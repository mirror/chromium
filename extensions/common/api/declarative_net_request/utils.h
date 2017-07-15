// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_UTILS_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_UTILS_H_

namespace base {
class FilePath;
}

namespace extensions {
namespace declarative_net_request {

// Indexes the json rules present at |json_rules_path| to the location specified
// by |indexed_ruleset_path|. In case of an error, returns false and populates
// |error|.
// Note: This must be called on a thread where file IO is allowed.
bool IndexAndPersistRuleset(const base::FilePath& json_rules_path,
                            const base::FilePath& indexed_ruleset_path,
                            std::string* error);

// Returns true if the Declarative Net Request API is available to the current
// environment.
bool IsAPIAvailable();

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_UTILS_H_
