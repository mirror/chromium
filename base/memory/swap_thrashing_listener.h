// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_SWAP_THRASHING_LISTENER_H_
#define BASE_MEMORY_SWAP_THRASHING_LISTENER_H_

#include "base/base_export.h"
#include "base/callback.h"
#include "base/macros.h"

namespace base {

class BASE_EXPORT SwapThrashingListener {
 public:
  enum SwapThrashingLevel {
    // No evidence of swap-thrashing.
    SWAP_THRASHING_LEVEL_NONE,

    // Swap-thrashing is suspected to affect the system at some levels.
    SWAP_THRASHING_LEVEL_MODERATE,

    // Swap-thrashing is seriously affecting the system.
    SWAP_THRASHING_LEVEL_CRITICAL,
  };

  typedef Callback<void(SwapThrashingLevel)> SwapThrashingCallback;
  typedef Callback<void(SwapThrashingLevel)> SyncSwapThrashingCallback;

  explicit SwapThrashingListener(
      const SwapThrashingCallback& swap_thrashing_callback);
  SwapThrashingListener(
      const SwapThrashingCallback& swap_thrashing_callback,
      const SyncSwapThrashingCallback& sync_swap_thrashing_callback);

  ~SwapThrashingListener();

  // Intended for use by the platform specific implementation.
  static void NotifySwapThrashing(SwapThrashingLevel swap_thrashing_level);

  void Notify(SwapThrashingLevel swap_thrashing_level);
  void SyncNotify(SwapThrashingLevel swap_thrashing_level);

 private:
  static void DoNotifySwapThrashing(SwapThrashingLevel swap_thrashing_level);

  SwapThrashingCallback callback_;
  SyncSwapThrashingCallback sync_swap_thrashing_callback_;

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingListener);
};

}  // namespace base

#endif  // BASE_MEMORY_SWAP_THRASHING_LISTENER_H_
