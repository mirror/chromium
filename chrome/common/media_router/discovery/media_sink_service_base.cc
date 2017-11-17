// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/discovery/media_sink_service_base.h"

#include <vector>

namespace {
// Time interval when media sink service sends sinks to MRP.
const int kFetchCompleteTimeoutSecs = 3;
}  // namespace

namespace media_router {

MediaSinkServiceBase::MediaSinkServiceBase(
    const OnSinksDiscoveredCallback& callback)
    : fetch_complete_timeout_secs_(kFetchCompleteTimeoutSecs),
      on_sinks_discovered_cb_(callback) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(!on_sinks_discovered_cb_.is_null());
}

MediaSinkServiceBase::~MediaSinkServiceBase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MediaSinkServiceBase::SetTimerForTest(std::unique_ptr<base::Timer> timer) {
  DCHECK(!finish_timer_);
  finish_timer_ = std::move(timer);
}

void MediaSinkServiceBase::OnFetchCompleted() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (current_sinks_ == mrp_sinks_) {
    DVLOG(2) << "No update to sink list.";
    return;
  }

  ForceSinkDiscoveryCallback();
  RecordDeviceCounts();
}

void MediaSinkServiceBase::StartTimer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Create a finish timer.
  if (!finish_timer_)
    finish_timer_.reset(new base::OneShotTimer());

  base::TimeDelta finish_delay =
      base::TimeDelta::FromSeconds(fetch_complete_timeout_secs_);
  finish_timer_->Start(FROM_HERE, finish_delay,
                       base::Bind(&MediaSinkServiceBase::OnFetchCompleted,
                                  base::Unretained(this)));
}

void MediaSinkServiceBase::StopTimer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  finish_timer_.reset();
}

void MediaSinkServiceBase::RestartTimer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!finish_timer_ || finish_timer_->IsRunning())
    return;
  StartTimer();
}

void MediaSinkServiceBase::ForceSinkDiscoveryCallback() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << "Send sinks to media router, [size]: " << current_sinks_.size();
  on_sinks_discovered_cb_.Run(std::vector<MediaSinkInternal>(
      current_sinks_.begin(), current_sinks_.end()));
  mrp_sinks_ = current_sinks_;
}

}  // namespace media_router
