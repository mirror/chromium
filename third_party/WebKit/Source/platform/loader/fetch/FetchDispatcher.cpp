// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/FetchDispatcher.h"

namespace blink {

namespace {

// 0u is unlimited.
size_t g_outstanding_limit = 4u;
}

FetchDispatcher::FetchDispatcher() {}

void FetchDispatcher::Request(Client* client, ThrottleOption option) {
  if (option == ThrottleOption::kCanNotBeThrottled)
    return Grant(client);

  pending_requests_.AppendOrMoveToLast(client);
  Wakeup();
}

bool FetchDispatcher::Release(Client* client) {
  if (running_requests_.find(client) == running_requests_.end())
    return false;
  running_requests_.erase(client);
  Wakeup();
  return true;
}

void FetchDispatcher::Wakeup() {
  if (pending_requests_.size() == 0 ||
      (g_outstanding_limit == 0 ||
       running_requests_.size() >= g_outstanding_limit)) {
    return;
  }
  Client* request = pending_requests_.front();
  DCHECK(request);
  pending_requests_.RemoveFirst();
  Grant(request);
}

void FetchDispatcher::Grant(Client* client) {
  running_requests_.insert(client);
  client->OnRequestGranted();
}

DEFINE_TRACE(FetchDispatcher) {
  visitor->Trace(running_requests_);
  visitor->Trace(pending_requests_);
}

}  // namespace blink
