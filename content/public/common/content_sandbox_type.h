// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_CONTENT_SANDBOX_TYPE_H_
#define CONTENT_COMMON_CONTENT_SANDBOX_TYPE_H_

#include "base/command_line.h"
#include "content/common/content_export.h"
#include "sandbox/policy/sandbox_type.h"

namespace content {

CONTENT_EXPORT void SetCommandLineFlagsForSandboxType(
    base::CommandLine* command_line,
    sandbox::SandboxType sandbox_type);

CONTENT_EXPORT sandbox::SandboxType SandboxTypeFromCommandLine(
    const base::CommandLine& command_line);

}  // namespace content

#endif  // CONTENT_COMMON_CONTENT_SANDBOX_TYPE_H_
