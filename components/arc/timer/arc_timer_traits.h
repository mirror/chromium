// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_

#include "components/arc/common/timer.mojom.h"
#include "components/arc/timer/arc_timer_request.h"

namespace mojo {

template <>
struct EnumTraits<arc::mojom::ClockId, int32_t> {
  static arc::mojom::ClockId ToMojom(int32_t clock_id);
  static bool FromMojom(arc::mojom::ClockId input, int32_t* output);
};

template <>
struct StructTraits<arc::mojom::ArcTimerRequestDataView, arc::ArcTimerRequest> {
  static int32_t clock_id(const arc::ArcTimerRequest& arc_timer_args);

  static mojo::ScopedHandle expiration_fd(
      const arc::ArcTimerRequest& arc_timer_args);

  static bool Read(arc::mojom::ArcTimerRequestDataView input,
                   arc::ArcTimerRequest* output);
};

}  // namespace mojo

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_
