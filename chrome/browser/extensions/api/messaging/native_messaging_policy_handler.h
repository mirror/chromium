// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_MESSAGING_NATIVE_MESSAGING_POLICY_HANDLER_H_
#define CHROME_BROWSER_EXTENSIONS_API_MESSAGING_NATIVE_MESSAGING_POLICY_HANDLER_H_

#include <memory>

#include "base/macros.h"
#include "base/values.h"
#include "components/policy/core/browser/configuration_policy_handler.h"

namespace extensions {

// Implements additional checks for policies that are lists of Native Messaging
// Hosts.
class NativeMessagingHostListPolicyHandler
    : public policy::StringListPolicyHandler {
 public:
  NativeMessagingHostListPolicyHandler(const char* policy_name,
                                       const char* pref_path,
                                       bool allow_wildcards);
  ~NativeMessagingHostListPolicyHandler() override;

 protected:
  // StringListPolicyHandler methods:

  // Checks whether |str| is indeed a valid host name (or a wildcard).
  bool CheckListEntry(const std::string& str) override;

 private:
  bool allow_wildcards_;

  DISALLOW_COPY_AND_ASSIGN(NativeMessagingHostListPolicyHandler);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_MESSAGING_NATIVE_MESSAGING_POLICY_HANDLER_H_
