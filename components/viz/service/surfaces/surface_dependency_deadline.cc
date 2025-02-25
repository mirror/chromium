// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/surfaces/surface_dependency_deadline.h"

#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/tick_clock.h"

namespace viz {

SurfaceDependencyDeadline::SurfaceDependencyDeadline(
    SurfaceDeadlineClient* client,
    BeginFrameSource* begin_frame_source,
    base::TickClock* tick_clock)
    : client_(client),
      begin_frame_source_(begin_frame_source),
      tick_clock_(tick_clock) {
  DCHECK(client_);
  DCHECK(begin_frame_source_);
  DCHECK(tick_clock_);
}

SurfaceDependencyDeadline::~SurfaceDependencyDeadline() {
  // The deadline must be canceled before destruction.
  DCHECK(!deadline_);
}

bool SurfaceDependencyDeadline::Set(base::TimeTicks frame_time,
                                    uint32_t number_of_frames_to_deadline,
                                    base::TimeDelta frame_interval) {
  DCHECK_GT(number_of_frames_to_deadline, 0u);
  CancelInternal(false);
  start_time_ = frame_time;
  deadline_ = start_time_ + number_of_frames_to_deadline * frame_interval;
  bool deadline_in_future = deadline_ > tick_clock_->NowTicks();
  begin_frame_source_->AddObserver(this);
  return deadline_in_future;
}

base::Optional<base::TimeDelta> SurfaceDependencyDeadline::Cancel() {
  return CancelInternal(false);
}

bool SurfaceDependencyDeadline::InheritFrom(
    const SurfaceDependencyDeadline& other) {
  if (*this == other)
    return false;

  DCHECK(has_deadline());

  CancelInternal(false);
  last_begin_frame_args_ = other.last_begin_frame_args_;
  begin_frame_source_ = other.begin_frame_source_;
  deadline_ = other.deadline_;
  if (deadline_)
    begin_frame_source_->AddObserver(this);
  return true;
}

bool SurfaceDependencyDeadline::operator==(
    const SurfaceDependencyDeadline& other) {
  return begin_frame_source_ == other.begin_frame_source_ &&
         deadline_ == other.deadline_;
}

// BeginFrameObserver implementation.
void SurfaceDependencyDeadline::OnBeginFrame(const BeginFrameArgs& args) {
  last_begin_frame_args_ = args;
  // OnBeginFrame might get called immediately after cancellation if some other
  // deadline triggered this deadline to be canceled.
  if (!deadline_)
    return;

  if (deadline_ > tick_clock_->NowTicks())
    return;

  base::Optional<base::TimeDelta> duration = CancelInternal(true);
  DCHECK(duration);

  client_->OnDeadline(*duration);
}

const BeginFrameArgs& SurfaceDependencyDeadline::LastUsedBeginFrameArgs()
    const {
  return last_begin_frame_args_;
}

bool SurfaceDependencyDeadline::WantsAnimateOnlyBeginFrames() const {
  return false;
}

void SurfaceDependencyDeadline::OnBeginFrameSourcePausedChanged(bool paused) {}

base::Optional<base::TimeDelta> SurfaceDependencyDeadline::CancelInternal(
    bool deadline) {
  if (!deadline_)
    return base::nullopt;

  begin_frame_source_->RemoveObserver(this);
  deadline_.reset();

  base::TimeDelta duration = tick_clock_->NowTicks() - start_time_;

  UMA_HISTOGRAM_TIMES("Compositing.SurfaceDependencyDeadline.Duration",
                      duration);

  UMA_HISTOGRAM_BOOLEAN("Compositing.SurfaceDependencyDeadline.DeadlineHit",
                        deadline);

  return duration;
}

}  // namespace viz
