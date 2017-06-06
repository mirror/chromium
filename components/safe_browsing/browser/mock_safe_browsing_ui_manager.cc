// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/browser/mock_safe_browsing_ui_manager.h"

#include <string>
namespace safe_browsing {

MockSafeBrowsingUIManager::MockSafeBrowsingUIManager()
    : BaseUIManager(), run_loop_(NULL) {}

// When the ThreatDetails is done, this is called.
void MockSafeBrowsingUIManager::SendSerializedThreatDetails(
    const std::string& serialized) {
  DVLOG(1) << "SendSerializedThreatDetails";
  run_loop_->Quit();
  run_loop_ = NULL;
  serialized_ = serialized;
}

// Used to synchronize SendSerializedThreatDetails() with
// WaitForSerializedReport(). RunLoop::RunUntilIdle() is not sufficient
// because the MessageLoop task queue completely drains at some point
// between the send and the wait.
void MockSafeBrowsingUIManager::SetRunLoopToQuit(base::RunLoop* run_loop) {
  DCHECK(run_loop_ == NULL);
  run_loop_ = run_loop;
}

const std::string& MockSafeBrowsingUIManager::GetSerialized() {
  return serialized_;
}

}  // namespace safe_browsing
