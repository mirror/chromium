// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/ResourceLoadScheduler.h"

namespace blink {

namespace {

// 0u is unlimited. Feature is completely disabled for now.
// TODO(crbug.com/735410): If this throttling is enabled always, it makes some
// tests fail.
size_t g_outstanding_limit = 0u;

}  // namespace

constexpr ResourceLoadScheduler::ClientId
    ResourceLoadScheduler::kInvalidClientId;

ResourceLoadScheduler::ResourceLoadScheduler() = default;

DEFINE_TRACE(ResourceLoadScheduler) {
  visitor->Trace(pending_request_map_);
}

void ResourceLoadScheduler::Request(ResourceLoadSchedulerClient* client,
                                    ThrottleOption option,
                                    ResourceLoadScheduler::ClientId* id) {
  *id = GenerateClientId();

  if (option == ThrottleOption::kCanNotBeThrottled)
    return Run(*id, client);

  pending_request_map_.insert(*id, client);
  pending_request_queue_.push_back(*id);
  MaybeRun();
}

bool ResourceLoadScheduler::Release(
    ResourceLoadScheduler::ClientId id,
    ResourceLoadScheduler::ReleaseOption option) {
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
    return true;
  }
  return false;
}

ResourceLoadScheduler::ClientId ResourceLoadScheduler::GenerateClientId() {
  ClientId id = ++current_id_;
  CHECK_NE(0u, id);
  return id;
}

void ResourceLoadScheduler::MaybeRun() {
  if (pending_request_queue_.empty() ||
      (g_outstanding_limit &&
       running_requests_.size() >= g_outstanding_limit)) {
    return;
  }
  while (!pending_request_queue_.empty()) {
    ClientId id = pending_request_queue_.TakeFirst();
    auto found = pending_request_map_.find(id);
    if (found == pending_request_map_.end())
      continue;  // Already released.
    ResourceLoadSchedulerClient* client = found->value;
    pending_request_map_.erase(found);
    Run(id, client);
    return;
  }
}

void ResourceLoadScheduler::Run(ResourceLoadScheduler::ClientId id,
                                ResourceLoadSchedulerClient* client) {
  running_requests_.insert(id);
  client->Run();
}

}  // namespace blink
