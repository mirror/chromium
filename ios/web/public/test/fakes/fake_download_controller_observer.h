// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_FAKES_FAKE_DOWNLOAD_CONTROLLER_OBSERVER_H_
#define IOS_WEB_PUBLIC_TEST_FAKES_FAKE_DOWNLOAD_CONTROLLER_OBSERVER_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "ios/web/public/download/download_controller_observer.h"

namespace web {

class DownloadController;

// DownloadControllerObserver which captures arguments passed to its callbacks.
class FakeDownloadControllerObserver : public DownloadControllerObserver {
 public:
  FakeDownloadControllerObserver(DownloadController* controller);
  ~FakeDownloadControllerObserver() override;

  // Returns downloads created via OnDownloadCreated and not yet destroyed.
  const std::vector<std::pair<const WebState*, scoped_refptr<DownloadTask>>>&
  alive_download_tasks() const {
    return alive_download_tasks_;
  }
  // Returns destroyed downloads.
  const std::vector<std::pair<const WebState*, DownloadTask*>>&
  destroyed_download_tasks() const {
    return destroyed_download_tasks_;
  }
  // Returns updated downloads.
  const std::vector<std::pair<const WebState*, DownloadTask*>>&
  updated_download_tasks() const {
    return updated_download_tasks_;
  }
  // Removes DownloadTask from the retust returned by GetAllDownloadTasks().
  // If the task is not referenced by any other variables, then the task will
  // be destroyed and will be returned from GetDestroyedDownloadTasks().
  void RemoveDownloadTask(DownloadTask* task);

 private:
  // DownloadControllerObserver overrides:
  void OnDownloadCreated(const WebState*, scoped_refptr<DownloadTask>) override;
  void OnDownloadUpdated(const WebState*, DownloadTask*) override;
  void OnDownloadDestroyed(const WebState*, DownloadTask*) override;
  void OnDownloadControllerDestroyed(DownloadController*) override;

  std::vector<std::pair<const WebState*, scoped_refptr<DownloadTask>>>
      alive_download_tasks_;
  std::vector<std::pair<const WebState*, DownloadTask*>>
      destroyed_download_tasks_;
  std::vector<std::pair<const WebState*, DownloadTask*>>
      updated_download_tasks_;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_FAKES_FAKE_DOWNLOAD_CONTROLLER_OBSERVER_H_
