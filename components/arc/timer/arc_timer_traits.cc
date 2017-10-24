// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/timer/arc_timer_traits.h"

#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace {

// Stub value.
constexpr int32_t kInvalidClockId = -1;

// Unwraps a mojo handle to a file descriptor on the system.
base::ScopedFD UnwrapPlatformHandle(mojo::ScopedHandle handle) {
  mojo::edk::ScopedPlatformHandle scoped_platform_handle;
  MojoResult mojo_result = mojo::edk::PassWrappedPlatformHandle(
      handle.release().value(), &scoped_platform_handle);
  if (mojo_result != MOJO_RESULT_OK) {
    VLOG(1) << "Failed to unwrap mojo handle: " << mojo_result;
    return base::ScopedFD();
  }
  return base::ScopedFD(scoped_platform_handle.release().handle);
}

}  // namespace

namespace mojo {

// static
int32_t clock_id(const arc::ArcTimerArgs& arc_timer_args) {
  // Stub function as the reverse translation is not required.
  return kInvalidClockId;
}

// static
mojo::ScopedHandle expiry_fd(const arc::ArcTimerArgs& arc_timer_args) {
  // Stub function as the reverse translation is not required.
  return mojo::ScopedHandle(Handle());
}

// static
bool StructTraits<arc::mojom::ArcTimerArgsDataView, arc::ArcTimerArgs>::Read(
    arc::mojom::ArcTimerArgsDataView input,
    arc::ArcTimerArgs* output) {
  output->clock_id = input.clock_id();
  output->expiration_fd = UnwrapPlatformHandle(input.TakeExpirationFd());
  return true;
}

}  // namespace mojo
