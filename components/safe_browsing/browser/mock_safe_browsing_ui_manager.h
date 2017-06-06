// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_BROWSER_MOCK_SAFE_BROWSING_UI_MANAGER_H_
#define COMPONENTS_SAFE_BROWSING_BROWSER_MOCK_SAFE_BROWSING_UI_MANAGER_H_

#include "base/run_loop.h"
#include "components/safe_browsing/base_ui_manager.h"

namespace safe_browsing {

class MockSafeBrowsingUIManager : public BaseUIManager {
 public:
  MockSafeBrowsingUIManager();

  // When the ThreatDetails is done, this is called.
  void SendSerializedThreatDetails(const std::string& serialized) override;

  // Used to synchronize SendSerializedThreatDetails() with
  // WaitForSerializedReport(). RunLoop::RunUntilIdle() is not sufficient
  // because the MessageLoop task queue completely drains at some point
  // between the send and the wait.
  void SetRunLoopToQuit(base::RunLoop* run_loop);

  // Returns the serialized report that was collected.
  const std::string& GetSerialized();

  base::RunLoop* run_loop_;

 private:
  ~MockSafeBrowsingUIManager() override {}

  std::string serialized_;
  DISALLOW_COPY_AND_ASSIGN(MockSafeBrowsingUIManager);
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_BROWSER_MOCK_SAFE_BROWSING_UI_MANAGER_H_
