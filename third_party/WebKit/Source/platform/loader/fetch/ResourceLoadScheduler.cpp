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

ResourceLoadScheduler::ResourceLoadScheduler() {}

void ResourceLoadScheduler::Request(ResourceLoadSchedulerClient* client,
                                    ThrottleOption option) {
  // Each |client| should not call Request twice.
  DCHECK(running_requests_.end() == running_requests_.find(client));
  DCHECK(pending_requests_.end() == pending_requests_.find(client));

  if (option == ThrottleOption::kCanNotBeThrottled)
    return Grant(client);

  pending_requests_.AppendOrMoveToLast(client);
  Wakeup();
}

bool ResourceLoadScheduler::Release(ResourceLoadSchedulerClient* client) {
  if (running_requests_.find(client) != running_requests_.end()) {
    running_requests_.erase(client);
    Wakeup();
    return true;
  }
  if (pending_requests_.find(client) != pending_requests_.end())
    pending_requests_.erase(client);
  return false;
}

void ResourceLoadScheduler::Wakeup() {
  if (pending_requests_.size() == 0 ||
      (g_outstanding_limit &&
       running_requests_.size() >= g_outstanding_limit)) {
    return;
  }
  ResourceLoadSchedulerClient* request = pending_requests_.front();
  DCHECK(request);
  pending_requests_.RemoveFirst();
  Grant(request);
}

void ResourceLoadScheduler::Grant(ResourceLoadSchedulerClient* client) {
  running_requests_.insert(client);
  client->OnRequestGranted();
}

DEFINE_TRACE(ResourceLoadScheduler) {
  visitor->Trace(running_requests_);
  visitor->Trace(pending_requests_);
}

}  // namespace blink
