// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/FetchDispatcher.h"

namespace blink {

FetchDispatcher::FetchDispatcher() {}

void FetchDispatcher::Request(Client* client) {
  running_requests_.insert(client);
  client->OnRequestDispatched();
}

bool FetchDispatcher::Release(Client* client) {
  if (running_requests_.find(client) == running_requests_.end())
    return false;
  running_requests_.erase(client);
  return true;
}

DEFINE_TRACE(FetchDispatcher) {}

}  // namespace blink
