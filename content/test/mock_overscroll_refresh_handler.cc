// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_overscroll_refresh_handler.h"

#include "content/public/test/test_utils.h"

namespace content {

MockOverscrollRefreshHandler::MockOverscrollRefreshHandler()
    : ui::OverscrollRefreshHandler(nullptr) {}

MockOverscrollRefreshHandler::~MockOverscrollRefreshHandler() {}

bool MockOverscrollRefreshHandler::PullStart() {
  // The first GestureScrollUpdate starts the pull, but does not update the
  // pull. For the purpose of testing, we'll be consistent with aura
  // overscroll and consider this an update.
  OnPullUpdate();
  return true;
}

void MockOverscrollRefreshHandler::PullUpdate(float) {
  OnPullUpdate();
}

void MockOverscrollRefreshHandler::PullRelease(bool) {
  OnPullEnd();
}

void MockOverscrollRefreshHandler::PullReset() {
  OnPullEnd();
}

void MockOverscrollRefreshHandler::WaitForUpdate() {
  if (!seen_update_)
    update_message_loop_runner_->Run();
}

void MockOverscrollRefreshHandler::WaitForEnd() {
  if (!pull_ended_)
    end_message_loop_runner_->Run();
}

void MockOverscrollRefreshHandler::Reset() {
  update_message_loop_runner_ = new MessageLoopRunner;
  end_message_loop_runner_ = new MessageLoopRunner;
  seen_update_ = false;
  pull_ended_ = false;
}

void MockOverscrollRefreshHandler::OnPullUpdate() {
  seen_update_ = true;
  if (update_message_loop_runner_->loop_running())
    update_message_loop_runner_->Quit();
}

void MockOverscrollRefreshHandler::OnPullEnd() {
  pull_ended_ = true;
  if (end_message_loop_runner_->loop_running())
    end_message_loop_runner_->Quit();
}

}  // namespace content
