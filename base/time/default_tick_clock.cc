// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/time/default_tick_clock.h"

#include "base/lazy_instance.h"

namespace base {
namespace default_tick_clock {
LazyInstance<DefaultTickClock>::Leaky g_instance = LAZY_INSTANCE_INITIALIZER;
}  // namespace default_tick_clock

DefaultTickClock::~DefaultTickClock() {}

TimeTicks DefaultTickClock::NowTicks() {
  return TimeTicks::Now();
}

// static
DefaultTickClock* DefaultTickClock::GetInstance() {
  return default_tick_clock::g_instance.Pointer();
}

}  // namespace base
