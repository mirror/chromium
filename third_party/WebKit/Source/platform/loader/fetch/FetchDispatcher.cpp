// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/FetchDispatcher.h"

namespace blink {

FetchDispatcher::FetchDispatcher() : active_requests_(0u) {}

void FetchDispatcher::AppendResourceToPendingQueue(Resource* resource) {
  pending_resources_.push_back(resource);
}

Resource* FetchDispatcher::GetNextAvailableResource() {
  if (pending_resources_.empty())
    return nullptr;

  Resource* resource = pending_resources_.front();
  DCHECK(resource);
  pending_resources_.pop_front();
  active_requests_++;
  DCHECK_NE(0u, active_requests_);
  return resource;
}

void FetchDispatcher::ReleaseCredit() {
  DCHECK_NE(0u, active_requests_);
  active_requests_--;
}

DEFINE_TRACE(FetchDispatcher) {
  visitor->Trace(pending_resources_);
}

}  // namespace blink
