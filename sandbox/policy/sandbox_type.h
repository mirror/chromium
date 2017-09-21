// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_POLICY_SANDBOX_TYPE_H_
#define SANDBOX_POLICY_SANDBOX_TYPE_H_

#include <string>

#include "sandbox/sandbox_export.h"

namespace sandbox {

// Defines the set of sandbox policies known to the system.

enum SandboxType {
  // Not a valid sandbox type.
  SANDBOX_TYPE_INVALID = -1,

  SANDBOX_TYPE_FIRST_TYPE = 0,  // Placeholder to ease iteration.

  // Do not apply any sandboxing to the process.
  SANDBOX_TYPE_NO_SANDBOX = SANDBOX_TYPE_FIRST_TYPE,

  // Renderer or worker process type. Most common case.
  SANDBOX_TYPE_RENDERER,

  // Utility type is as restrictive as the worker process except full
  // access is allowed to one configurable directory.
  SANDBOX_TYPE_UTILITY,

  // GPU process type.
  SANDBOX_TYPE_GPU,

  // The PPAPI plugin process type
  SANDBOX_TYPE_PPAPI,

  // The network service type.
  SANDBOX_TYPE_NETWORK,

  // The CDM service type.
  SANDBOX_TYPE_WIDEVINE,

  SANDBOX_TYPE_AFTER_LAST_TYPE,  // Placeholder to ease iteration.
};

// Determines if a given sandbox type is completely unsandboxed. Currently
// this is the same across all platforms, but we'd expect that there may
// be platform-specific exceptions introduced in the future.
SANDBOX_EXPORT bool IsUnsandboxedSandboxType(SandboxType sandbox_type);

// Sandbox types, as contained in things like service manager manifests or
// command lines, need to be represented by strings. Provide a consistent
// means of mapping back and forth.
SANDBOX_EXPORT std::string SandboxTypeToString(SandboxType sandbox_type);
SANDBOX_EXPORT SandboxType
SandboxTypeFromString(const std::string& sandbox_string);

}  // namespace sandbox

#endif  // SANDBOX_TYPE_SANDBOX_TYPE_H_
