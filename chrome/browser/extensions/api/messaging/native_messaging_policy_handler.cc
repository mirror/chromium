// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/messaging/native_messaging_policy_handler.h"

#include "chrome/browser/extensions/api/messaging/native_messaging_host_manifest.h"

namespace extensions {

NativeMessagingHostListPolicyHandler::NativeMessagingHostListPolicyHandler(
    const char* policy_name,
    const char* pref_path,
    bool allow_wildcards)
    : policy::StringListPolicyHandler(policy_name, pref_path),
      allow_wildcards_(allow_wildcards) {}

NativeMessagingHostListPolicyHandler::~NativeMessagingHostListPolicyHandler() {}

bool NativeMessagingHostListPolicyHandler::CheckListEntry(
    const std::string& str) {
  if (allow_wildcards_ && str == "*")
    return true;

  return NativeMessagingHostManifest::IsValidName(str);
}

}  // namespace extensions
