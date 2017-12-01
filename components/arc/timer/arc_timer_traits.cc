// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/timer/arc_timer_traits.h"

#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace {

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
  LOG(ERROR) << "Clock: " << clock_id;
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
int32_t StructTraits<arc::mojom::ArcTimerRequestDataView,
                     arc::ArcTimerRequest>::clock_id(const arc::ArcTimerRequest&
                                                         arc_timer_request) {
  // Stub function as the reverse translation is not required.
  LOG(ERROR) << "X: " << arc_timer_request.clock_id;
  switch (static_cast<int32_t>(arc_timer_request.clock_id)) {
    case static_cast<int32_t>(arc::mojom::ClockId::REALTIME_ALARM):
      return CLOCK_REALTIME_ALARM;
    case static_cast<int32_t>(arc::mojom::ClockId::BOOTTIME_ALARM):
      return CLOCK_BOOTTIME_ALARM;
  }
  NOTREACHED();
  return CLOCK_BOOTTIME_ALARM;
}

// static
mojo::ScopedHandle
StructTraits<arc::mojom::ArcTimerRequestDataView, arc::ArcTimerRequest>::
    expiration_fd(const arc::ArcTimerRequest& arc_timer_request) {
  LOG(ERROR) << "Y";
  // Stub function as the reverse translation is not required.
  return mojo::ScopedHandle(Handle());
}

// static
bool StructTraits<arc::mojom::ArcTimerRequestDataView, arc::ArcTimerRequest>::
    Read(arc::mojom::ArcTimerRequestDataView input,
         arc::ArcTimerRequest* output) {
  LOG(ERROR) << "Z";
  if (!EnumTraits<arc::mojom::ClockId, int32_t>::FromMojom(input.clock_id(),
                                                           &output->clock_id)) {
    return false;
  }
  output->expiration_fd = UnwrapPlatformHandle(input.TakeExpirationFd());
  return true;
}

}  // namespace mojo
