// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/status_change_checker.h"

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/timer/timer.h"

StatusChangeChecker::StatusChangeChecker() : timed_out_(false) {}

StatusChangeChecker::~StatusChangeChecker() {}

bool StatusChangeChecker::Wait() {
  if (IsExitConditionSatisfied()) {
    LOG(INFO) << "Already satisfied: " << GetDebugMessage();
  } else {
    LOG(INFO) << "Blocking: " << GetDebugMessage();
    StartBlockingWait();
  }
  return !TimedOut();
}

bool StatusChangeChecker::TimedOut() const {
  return timed_out_;
}

base::TimeDelta StatusChangeChecker::GetTimeoutDuration() {
  return base::TimeDelta::FromSeconds(45);
}

void StatusChangeChecker::StartBlockingWait() {
  base::OneShotTimer timer;
  timer.Start(FROM_HERE,
              GetTimeoutDuration(),
              base::Bind(&StatusChangeChecker::OnTimeout,
                         base::Unretained(this)));

  LOG(INFO) << "StatusChangeChecker::StartBlockingWait() Start";
  base::RunLoop(base::RunLoop::Type::kNestableTasksAllowed).Run();
  LOG(INFO) << "StatusChangeChecker::StartBlockingWait() End";
}

void StatusChangeChecker::StopWaiting() {
  LOG(INFO) << "QuitCurrent() due to StatusChangeChecker::StopWaiting()";
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

void StatusChangeChecker::CheckExitCondition() {
  LOG(INFO) << "Await -> Checking Condition: " << GetDebugMessage();
  if (IsExitConditionSatisfied()) {
    LOG(INFO) << "Await -> Condition met: " << GetDebugMessage();
    StopWaiting();
  }
}

void StatusChangeChecker::OnTimeout() {
  LOG(INFO) << "StatusChangeChecker::OnTimeout()";
  DVLOG(1) << "Await -> Timed out: " << GetDebugMessage();
  timed_out_ = true;
  StopWaiting();
}
