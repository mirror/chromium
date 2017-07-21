// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_ANDROID_PREFETCH_BACKGROUND_TASK_HANDLER_IMPL_H_
#define CHROME_BROWSER_OFFLINE_PAGES_ANDROID_PREFETCH_BACKGROUND_TASK_HANDLER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "components/offline_pages/core/prefetch/prefetch_background_task_handler.h"

class Profile;

namespace net {
class BackoffEntry;
}

namespace offline_pages {

class PrefetchBackgroundTaskHandlerImpl : public PrefetchBackgroundTaskHandler {
 public:
  explicit PrefetchBackgroundTaskHandlerImpl(Profile* profile);
  ~PrefetchBackgroundTaskHandlerImpl() override;

  void CancelBackgroundTask() override;
  void EnsureTaskScheduled() override;

  std::unique_ptr<net::BackoffEntry> GetCurrentBackoff();
  void Backoff();
  void ResetBackoff();
  int GetAdditionalBackoffSeconds();

 private:
  void UpdateBackoff(net::BackoffEntry* backoff);
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchBackgroundTaskHandlerImpl);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_ANDROID_PREFETCH_BACKGROUND_TASK_HANDLER_IMPL_H_
