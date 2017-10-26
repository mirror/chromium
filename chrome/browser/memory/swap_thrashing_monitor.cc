// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/memory/swap_thrashing_monitor.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"

namespace memory {
namespace {

constexpr base::TimeDelta kSamplingInterval = base::TimeDelta::FromSeconds(2);

// Enumeration of UMA swap thrashing levels. This needs to be kept in sync with
// tools/metrics/histograms/enums.xml and the swap_thrashing levels defined in
// swap_thrashing_common.h.
enum SwapThrashingLevelUMA {
  UMA_SWAP_THRASHING_LEVEL_NONE = 0,
  UMA_SWAP_THRASHING_LEVEL_SUSPECTED = 1,
  UMA_SWAP_THRASHING_LEVEL_CONFIRMED = 2,
  // This must be the last value in the enum.
  UMA_SWAP_THRASHING_LEVEL_COUNT,
};

// Enumeration of UMA swap thrashing level changes. This needs to be kept in
// sync with tools/metrics/histograms/enums.xml.
enum SwapThrashingLevelChangesUMA {
  UMA_SWAP_THRASHING_LEVEL_CHANGE_NONE_TO_SUSPECTED = 0,
  UMA_SWAP_THRASHING_LEVEL_CHANGE_SUSPECTED_TO_CONFIRMED = 1,
  UMA_SWAP_THRASHING_LEVEL_CHANGE_CONFIRMED_TO_SUSPECTED = 2,
  UMA_SWAP_THRASHING_LEVEL_CHANGE_SUSPECTED_TO_NONE = 3,
  // This must be the last value in the enum.
  UMA_SWAP_THRASHING_LEVEL_CHANGE_COUNT,
};

// Converts a swap thrashing level to an UMA enumeration value.
SwapThrashingLevelUMA SwapThrashingLevelToUmaEnumValue(
    SwapThrashingLevel level) {
  switch (level) {
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE:
      return UMA_SWAP_THRASHING_LEVEL_NONE;
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED:
      return UMA_SWAP_THRASHING_LEVEL_SUSPECTED;
    case SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED:
      return UMA_SWAP_THRASHING_LEVEL_CONFIRMED;
  }
  NOTREACHED();
  return UMA_SWAP_THRASHING_LEVEL_NONE;
}

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

SwapThrashingMonitor::SwapThrashingMonitor()
    : delegate_(base::MakeUnique<SwapThrashingMonitorDelegate>()),
      current_swap_thrashing_level_(
          SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE) {
  StartObserving();
}

SwapThrashingMonitor::~SwapThrashingMonitor() {}

SwapThrashingLevel SwapThrashingMonitor::GetCurrentSwapThrashingLevel() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return current_swap_thrashing_level_;
}

void SwapThrashingMonitor::CheckSwapThrashingPressureAndRecordStatistics() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  SwapThrashingLevel swap_thrashing_level =
      delegate_->SampleAndCalculateSwapThrashingLevel();

  // Record the state changes.
  if (swap_thrashing_level != current_swap_thrashing_level_) {
    SwapThrashingLevelChangesUMA level_change =
        UMA_SWAP_THRASHING_LEVEL_CHANGE_COUNT;

    switch (current_swap_thrashing_level_) {
      case SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE: {
        DCHECK_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED,
                  swap_thrashing_level);
        level_change = UMA_SWAP_THRASHING_LEVEL_CHANGE_NONE_TO_SUSPECTED;
        break;
      }
      case SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED: {
        if (swap_thrashing_level ==
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE) {
          level_change = UMA_SWAP_THRASHING_LEVEL_CHANGE_SUSPECTED_TO_NONE;
        } else {
          level_change = UMA_SWAP_THRASHING_LEVEL_CHANGE_SUSPECTED_TO_CONFIRMED;
        }
        break;
      }
      case SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED: {
        DCHECK_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED,
                  swap_thrashing_level);
        level_change = UMA_SWAP_THRASHING_LEVEL_CHANGE_CONFIRMED_TO_SUSPECTED;
        break;
      }
      default:
        break;
    }
    STATIC_HISTOGRAM_POINTER_BLOCK(
        "Memory.SwapThrashingLevelChanges", AddCount(level_change, 1),
        base::LinearHistogram::FactoryGet(
            "Memory.SwapThrashingLevelChanges", 1,
            UMA_SWAP_THRASHING_LEVEL_CHANGE_COUNT,
            UMA_SWAP_THRASHING_LEVEL_CHANGE_COUNT + 1,
            base::HistogramBase::kUmaTargetedHistogramFlag));
  }

  current_swap_thrashing_level_ = swap_thrashing_level;

  // Record the current state.
  STATIC_HISTOGRAM_POINTER_BLOCK(
      "Memory.SwapThrashingLevel",
      AddCount(SwapThrashingLevelToUmaEnumValue(current_swap_thrashing_level_),
               1),
      base::LinearHistogram::FactoryGet(
          "Memory.SwapThrashingLevel", 1, UMA_SWAP_THRASHING_LEVEL_COUNT,
          UMA_SWAP_THRASHING_LEVEL_COUNT + 1,
          base::HistogramBase::kUmaTargetedHistogramFlag));
}

void SwapThrashingMonitor::StartObserving() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(sebmarchand): Determine if the system is using on-disk swap, abort if
  // it isn't as there won't be any swap-paging to observe (on-disk swap could
  // later become available if the user turn it on but this case is rare that
  // it's safe to ignore it).
  timer_.Start(
      FROM_HERE, kSamplingInterval,
      base::Bind(
          &SwapThrashingMonitor::CheckSwapThrashingPressureAndRecordStatistics,
          base::Unretained(this)));
}

}  // namespace memory
