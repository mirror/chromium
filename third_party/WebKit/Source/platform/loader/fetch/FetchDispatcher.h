// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FetchDispatcher_h
#define FetchDispatcher_h

#include "platform/heap/GarbageCollected.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/loader/fetch/Resource.h"

namespace blink {

// The FetchDispatcher provides an unified infrastructure to give the loader
// more controls to organize loading request issues. The ResourceFetcher owns
// a FetchDispatcher instance to control loading requests that ResourceFetcher
// manages per-context basis.
class FetchDispatcher final : public GarbageCollected<FetchDispatcher> {
  WTF_MAKE_NONCOPYABLE(FetchDispatcher);

 public:
  static FetchDispatcher* Create() { return new FetchDispatcher(); }
  DECLARE_VIRTUAL_TRACE();

  // Appends new Resource instance to the pending resource request queue.
  void AppendResourceToPendingQueue(Resource*);

  // Obtains the first Resource instance in the request queue if there is in the
  // queue, and have a credit to start a new request. It will return nullptr
  // if it isn't available. Otherwise, it consumes a credit to run a resource
  // request.
  Resource* GetNextAvailableResource();

  // Release a credit to run a resource request.
  void ReleaseCredit();

 private:
  FetchDispatcher();

  // Holds all appended Resource instances in the first-in first-out order.
  HeapDeque<Member<Resource>> pending_resources_;

  // Counts running resource requests to mange credits.
  size_t active_requests_;
};

}  // namespace blink

#endif
