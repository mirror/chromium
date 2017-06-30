// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/ResourceLoadScheduler.h"

#include "platform/RuntimeEnabledFeatures.h"

namespace blink {

namespace {

// TODO(toyoshim): Should be managed via field trial flag.
constexpr size_t kOutstandingThrottledLimit = 16u;

}  // namespace

constexpr ResourceLoadScheduler::ClientId
    ResourceLoadScheduler::kInvalidClientId;

ResourceLoadScheduler::ResourceLoadScheduler(FetchContext* context)
    : context_(context) {
  DCHECK(context);
  if (RuntimeEnabledFeatures::ResourceLoadSchedulerEnabled()) {
    auto* scheduler = context->GetScheduler();
    if (scheduler) {
      state_ = State::kActive;
      scheduler->AddThrottlingObserver(WebFrameScheduler::ObserverType::kLoader,
                                       this);
    }
  }
}

DEFINE_TRACE(ResourceLoadScheduler) {
  visitor->Trace(pending_request_map_);
  visitor->Trace(context_);
}

void ResourceLoadScheduler::Shutdown() {
  if (state_ == State::kActive) {
    auto* scheduler = context_->GetScheduler();
    if (scheduler) {
      scheduler->RemoveThrottlingObserver(
          WebFrameScheduler::ObserverType::kLoader, this);
    }
  }
  state_ = State::kShutdown;
}

void ResourceLoadScheduler::Request(ResourceLoadSchedulerClient* client,
                                    ThrottleOption option,
                                    ResourceLoadScheduler::ClientId* id) {
  *id = GenerateClientId();

  if (state_ == State::kInactive ||
      option == ThrottleOption::kCanNotBeThrottled) {
    Run(*id, client);
    return;
  }

  pending_request_map_.insert(*id, client);
  pending_request_queue_.push_back(*id);
  MaybeRun();
}

bool ResourceLoadScheduler::Release(
    ResourceLoadScheduler::ClientId id,
    ResourceLoadScheduler::ReleaseOption option) {
  // Check kInvalidClientId that can not be passed to the HashSet.
  if (id == kInvalidClientId)
    return false;

  if (running_requests_.find(id) != running_requests_.end()) {
    running_requests_.erase(id);
    if (option == ReleaseOption::kReleaseAndSchedule)
      MaybeRun();
    return true;
  }
  auto found = pending_request_map_.find(id);
  if (found != pending_request_map_.end()) {
    pending_request_map_.erase(found);
    // Intentionally does not remove it from |pending_request_queue_|.

    // Didn't release any running request, but the outstanding limit might be
    // changed to allow another request.
    if (option == ReleaseOption::kReleaseAndSchedule)
      MaybeRun();
    return true;
  }
  return false;
}

void ResourceLoadScheduler::SetOutstandingLimitForTesting(size_t limit) {
  state_ = State::kActive;
  SetOutstandingLimitAndMaybeRun(limit);
}

void ResourceLoadScheduler::OnThrottlingStateChanged(
    WebFrameScheduler::ThrottlingState state) {
  switch (state) {
    case WebFrameScheduler::ThrottlingState::kThrottled:
      SetOutstandingLimitAndMaybeRun(kOutstandingThrottledLimit);
      break;
    case WebFrameScheduler::ThrottlingState::kNotThrottled:
      SetOutstandingLimitAndMaybeRun(kOutstandingUnlimited);
      break;
  }
}

ResourceLoadScheduler::ClientId ResourceLoadScheduler::GenerateClientId() {
  ClientId id = ++current_id_;
  CHECK_NE(0u, id);
  return id;
}

void ResourceLoadScheduler::MaybeRun() {
  if (state_ != State::kActive)
    return;

  while (!pending_request_queue_.empty()) {
    if (outstanding_limit_ && running_requests_.size() >= outstanding_limit_)
      return;
    ClientId id = pending_request_queue_.TakeFirst();
    auto found = pending_request_map_.find(id);
    if (found == pending_request_map_.end())
      continue;  // Already released.
    ResourceLoadSchedulerClient* client = found->value;
    pending_request_map_.erase(found);
    Run(id, client);
  }
}

void ResourceLoadScheduler::Run(ResourceLoadScheduler::ClientId id,
                                ResourceLoadSchedulerClient* client) {
  running_requests_.insert(id);
  client->Run();
}

void ResourceLoadScheduler::SetOutstandingLimitAndMaybeRun(size_t limit) {
  outstanding_limit_ = limit;
  MaybeRun();
}

}  // namespace blink
