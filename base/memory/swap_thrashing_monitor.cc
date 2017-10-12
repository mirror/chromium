// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/swap_thrashing_monitor.h"

#include "base/logging.h"

namespace base {
namespace {

// The global SwapThrashingMonitor instance.
SwapThrashingMonitor* g_instance = nullptr;

}  // namespace

// static
void SwapThrashingMonitor::SetInstance(
    std::unique_ptr<SwapThrashingMonitor> instance) {
  DCHECK_EQ(nullptr, g_instance);
  g_instance = instance.release();
}

SwapThrashingMonitor* SwapThrashingMonitor::GetInstance() {
  return g_instance;
}

}  // namespace base
