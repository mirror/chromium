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
arc::mojom::ClockId EnumTraits<arc::mojom::ClockId, int32_t>::ToMojom(
    int32_t clock_id) {
  switch (clock_id) {
    case CLOCK_REALTIME_ALARM:
      return arc::mojom::ClockId::REALTIME_ALARM;
    case CLOCK_BOOTTIME_ALARM:
      return arc::mojom::ClockId::BOOTTIME_ALARM;
  }
  NOTREACHED();
  return arc::mojom::ClockId::BOOTTIME_ALARM;
}

// static
bool EnumTraits<arc::mojom::ClockId, int32_t>::FromMojom(
    arc::mojom::ClockId input,
    int32_t* output) {
  switch (input) {
    case arc::mojom::ClockId::REALTIME_ALARM:
      *output = CLOCK_REALTIME_ALARM;
      return true;
    case arc::mojom::ClockId::BOOTTIME_ALARM:
      *output = CLOCK_BOOTTIME_ALARM;
      return true;
  }
  NOTREACHED();
  return false;
}

// static
int32_t clock_id(const arc::ArcTimerRequest& arc_timer_args) {
  // Stub function as the reverse translation is not required.
  return kInvalidClockId;
}

// static
mojo::ScopedHandle expiration_fd(const arc::ArcTimerRequest& arc_timer_args) {
  // Stub function as the reverse translation is not required.
  return mojo::ScopedHandle(Handle());
}

// static
bool StructTraits<arc::mojom::ArcTimerRequestDataView, arc::ArcTimerRequest>::
    Read(arc::mojom::ArcTimerRequestDataView input,
         arc::ArcTimerRequest* output) {
  if (!EnumTraits<arc::mojom::ClockId, int32_t>::FromMojom(input.clock_id(),
                                                           &output->clock_id)) {
    return false;
  }
  output->expiration_fd = UnwrapPlatformHandle(input.TakeExpirationFd());
  return true;
}

}  // namespace mojo
