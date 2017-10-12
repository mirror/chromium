// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/timer/arc_timer_traits.h"

#include <mojo/edk/embedder/embedder.h>
#include <mojo/edk/embedder/scoped_platform_handle.h>

namespace {

// Unwraps a mojo handle to a file descriptor on the system.
int UnwrapPlatformHandle(mojo::ScopedHandle handle) {
  mojo::edk::ScopedPlatformHandle scoped_platform_handle;
  MojoResult mojo_result = mojo::edk::PassWrappedPlatformHandle(
      handle.release().value(), &scoped_platform_handle);
  if (mojo_result != MOJO_RESULT_OK) {
    VLOG(1) << "Failed to unwrap mojo handle: " << mojo_result;
    return -EINVAL;
  }
  return scoped_platform_handle.release().handle;
}

}  // namespace

namespace mojo {

// static
bool StructTraits<arc::mojom::ArcTimerArgsDataView,
                  chromeos::PowerManagerClient::ArcTimerArgs>::
    Read(arc::mojom::ArcTimerArgsDataView input,
         chromeos::PowerManagerClient::ArcTimerArgs* output) {
  output->clock_id = input.clock_id();
  output->expiry_indicator_fd =
      base::ScopedFD(UnwrapPlatformHandle(input.TakeExpiryIndicatorFd()));
  return true;
}
}  // namespace mojo
