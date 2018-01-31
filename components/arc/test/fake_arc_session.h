// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TEST_FAKE_ARC_SESSION_H_
#define COMPONENTS_ARC_TEST_FAKE_ARC_SESSION_H_

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "components/arc/arc_session.h"
#include "components/arc/arc_stop_reason.h"

namespace arc {

// A fake ArcSession that creates a local connection.
class FakeArcSession : public ArcSession {
 public:
  FakeArcSession();
  ~FakeArcSession() override;

  // ArcSession overrides:
  void StartMiniInstance() override;
  void RequestUpgrade() override;
  void Stop() override;
  bool IsStopRequested() override;
  void OnShutdown() override;

  // To emulate unexpected stop, such as crash.
  void StopWithReason(ArcStopReason reason);

  // To simulate the container progressing to it's requested state, or
  // optionally, failing while trying.
  void SimulateExecution(base::Optional<ArcStopReason> failure_reason);

  // Returns true if the session is considered as running.
  bool is_running() const { return running_; }

  // Returns FakeArcSession instance. This can be used for a factory
  // in ArcBridgeServiceImpl.
  static std::unique_ptr<ArcSession> Create();

 private:
  bool upgrade_requested_ = false;
  bool running_ = false;
  bool stop_requested_ = false;

  DISALLOW_COPY_AND_ASSIGN(FakeArcSession);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TEST_FAKE_ARC_SESSION_H_
