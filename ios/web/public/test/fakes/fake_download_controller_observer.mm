// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/test/fakes/fake_download_controller_observer.h"

#include "ios/web/public/download/download_controller.h"
#include "ios/web/public/download/download_task.h"

namespace web {

FakeDownloadControllerObserver::FakeDownloadControllerObserver(
    DownloadController* controller) {
  controller->AddObserver(this);
}

FakeDownloadControllerObserver::~FakeDownloadControllerObserver() = default;

void FakeDownloadControllerObserver::RemoveDownloadTask(DownloadTask* task) {
  for (auto it = alive_download_tasks_.begin();
       it != alive_download_tasks_.end(); ++it) {
    if (it->second.get() == task) {
      alive_download_tasks_.erase(it);
      break;
    }
  }
}

void FakeDownloadControllerObserver::OnDownloadCreated(
    const WebState* web_state,
    scoped_refptr<DownloadTask> task) {
  alive_download_tasks_.push_back(std::make_pair(web_state, task));
}

void FakeDownloadControllerObserver::OnDownloadUpdated(
    const WebState* web_state,
    DownloadTask* task) {
  updated_download_tasks_.push_back(std::make_pair(web_state, task));
}

void FakeDownloadControllerObserver::OnDownloadDestroyed(
    const WebState* web_state,
    DownloadTask* task) {
  destroyed_download_tasks_.push_back(std::make_pair(web_state, task));
}

void FakeDownloadControllerObserver::OnDownloadControllerDestroyed(
    DownloadController* controller) {
  controller->RemoveObserver(this);
}

}  // namespace web
