// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_PLATFORM_HANDLE_VECTOR_H_
#define MOJO_EDK_EMBEDDER_PLATFORM_HANDLE_VECTOR_H_

#include <vector>

#include "mojo/edk/embedder/platform_handle.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace mojo {
namespace edk {

using PlatformHandleVector = std::vector<PlatformHandle>;
using ScopedPlatformHandleVectorPtr = std::vector<ScopedPlatformHandle>;

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_PLATFORM_HANDLE_VECTOR_H_
