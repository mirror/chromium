// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_BACKGROUND_TASK_HANDLER_IMPL_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_BACKGROUND_TASK_HANDLER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/time/tick_clock.h"
#include "components/offline_pages/core/prefetch/prefetch_background_task_handler.h"

class PrefService;

namespace base {
class TickClock;
}

namespace net {
class BackoffEntry;
}

namespace offline_pages {

// A class living on the PrefetchService that is able to ask the Android
// background task system to schedule or cancel our task.  Also manages the
// backoff, so we can schedule tasks at the appropriate time based on the
// backoff value.
// TODO(dewittj): Split this class into two - one that handles scheduling and
// one that handles backoff, since backoff is platform-independent.
class PrefetchBackgroundTaskHandlerImpl : public PrefetchBackgroundTaskHandler {
 public:
  explicit PrefetchBackgroundTaskHandlerImpl(PrefService* profile);
  ~PrefetchBackgroundTaskHandlerImpl() override;

  // PrefetchBackgroundTaskHandler implementation.
  void CancelBackgroundTask() override;
  void EnsureTaskScheduled() override;

  // Backoff control.  These functions directly modify/read prefs.
  std::unique_ptr<net::BackoffEntry> GetCurrentBackoff();
  void Backoff() override;
  void ResetBackoff() override;
  int GetAdditionalBackoffSeconds() override;

  // This is used to construct the backoff value.
  void SetTickClockForTest(base::TickClock* clock);

 private:
  void UpdateBackoff(net::BackoffEntry* backoff);
  PrefService* prefs_;
  base::TickClock* clock_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchBackgroundTaskHandlerImpl);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_BACKGROUND_TASK_HANDLER_IMPL_H_
