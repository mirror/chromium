// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/test/fake_arc_session.h"

#include <memory>

#include "base/logging.h"

namespace arc {

FakeArcSession::FakeArcSession() = default;

FakeArcSession::~FakeArcSession() = default;

void FakeArcSession::StartMiniInstance() {}

void FakeArcSession::RequestUpgrade() {
  upgrade_requested_ = true;
}

void FakeArcSession::Stop() {
  stop_requested_ = true;
  StopWithReason(ArcStopReason::SHUTDOWN);
}

bool FakeArcSession::IsStopRequested() {
  return stop_requested_;
}

void FakeArcSession::OnShutdown() {
  StopWithReason(ArcStopReason::SHUTDOWN);
}

void FakeArcSession::StopWithReason(ArcStopReason reason) {
  bool was_mojo_connected = running_;
  running_ = false;
  for (auto& observer : observer_list_)
    observer.OnSessionStopped(reason, was_mojo_connected, upgrade_requested_);
}

void FakeArcSession::SimulateExecution(
    base::Optional<ArcStopReason> failure_reason) {
  if (failure_reason) {
    for (auto& observer : observer_list_)
      observer.OnSessionStopped(failure_reason.value(), false,
                                upgrade_requested_);
  } else if (upgrade_requested_) {
    running_ = true;
  }
}

// static
std::unique_ptr<ArcSession> FakeArcSession::Create() {
  return std::make_unique<FakeArcSession>();
}

}  // namespace arc
