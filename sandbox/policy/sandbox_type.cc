// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/policy/sandbox_type.h"

#include <string>

namespace sandbox {

namespace {

// These match the allowed "sandbox_type" values in service_manager's
// manifest.json files.
const char kNoSandboxType[] = "none";
const char kNetworkSandboxType[] = "network";
const char kPpapiSandboxType[] = "ppapi";
const char kUtilitySandboxType[] = "utility";
const char kWidevineSandboxType[] = "widevine";

}  // namespace

SANDBOX_EXPORT bool IsUnsandboxedSandboxType(SandboxType sandbox_type) {
  // TODO(tsepez): Sandbox network service.
  return sandbox_type == SANDBOX_TYPE_NO_SANDBOX ||
         sandbox_type == SANDBOX_TYPE_NETWORK;
}

std::string SandboxTypeToString(SandboxType sandbox_type) {
  switch (sandbox_type) {
    case SANDBOX_TYPE_NO_SANDBOX:
      return kNoSandboxType;
    case SANDBOX_TYPE_NETWORK:
      return kNetworkSandboxType;
    case SANDBOX_TYPE_PPAPI:
      return kPpapiSandboxType;
    case SANDBOX_TYPE_WIDEVINE:
      return kWidevineSandboxType;
    default:
      return kUtilitySandboxType;
  }
}

SandboxType SandboxTypeFromString(const std::string& sandbox_string) {
  if (sandbox_string == kNoSandboxType)
    return SANDBOX_TYPE_NO_SANDBOX;
  if (sandbox_string == kNetworkSandboxType)
    return SANDBOX_TYPE_NETWORK;
  if (sandbox_string == kPpapiSandboxType)
    return SANDBOX_TYPE_PPAPI;
  if (sandbox_string == kWidevineSandboxType)
    return SANDBOX_TYPE_WIDEVINE;
  return SANDBOX_TYPE_UTILITY;
}

}  // namespace sandbox
