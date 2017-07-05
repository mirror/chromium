// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_TRIGGERS_TRIGGER_THROTTLER_H_
#define COMPONENTS_SAFE_BROWSING_TRIGGERS_TRIGGER_THROTTLER_H_

#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "base/time/time.h"

namespace safe_browsing {

enum class SafeBrowsingTriggerType {
  SECURITY_INTERSTITIAL = 1,
};

struct SafeBrowsingTriggerTypeHash {
  std::size_t operator()(SafeBrowsingTriggerType trigger_type) const {
    return static_cast<std::size_t>(trigger_type);
  };
};

using TriggerTimestampMap = std::unordered_map<SafeBrowsingTriggerType,
                                               std::vector<time_t>,
                                               SafeBrowsingTriggerTypeHash>;

class TriggerThrottler {
 public:
  TriggerThrottler();
  virtual ~TriggerThrottler();

  bool CheckQuota(const SafeBrowsingTriggerType type) const;
  void TriggerFired(const SafeBrowsingTriggerType type);

 private:
  void CleanupOldEvents();

  TriggerTimestampMap trigger_events_;
  DISALLOW_COPY_AND_ASSIGN(TriggerThrottler);
};

}  // namespace safe_browsing
// namespace safe_browsing
#endif  // COMPONENTS_SAFE_BROWSING_TRIGGERS_TRIGGER_THROTTLER_H_
