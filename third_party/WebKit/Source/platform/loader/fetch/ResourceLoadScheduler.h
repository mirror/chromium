// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ResourceLoadScheduler_h
#define ResourceLoadScheduler_h

#include "platform/heap/GarbageCollected.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/loader/fetch/Resource.h"
#include "platform/wtf/Deque.h"
#include "platform/wtf/HashSet.h"

namespace blink {

// Client interface to use the arbitration functionality that
// ResourceLoadScheduler provides.
class PLATFORM_EXPORT ResourceLoadSchedulerClient {
 public:
  // Called when the request is granted to run.
  virtual void Run() = 0;
};

// The ResourceLoadScheduler provides a unified per-frame infrastructure to
// schedule loading requests. It receives resource requests as
// ResourceLoadSchedulerClient instances and calls Run() on them to dispatch
// them possibly with additional throttling/scheduling. This also keeps track of
// in-flight requests that are granted but are not released (by Release()) yet.
class ResourceLoadScheduler final
    : public GarbageCollectedFinalized<ResourceLoadScheduler> {
  WTF_MAKE_NONCOPYABLE(ResourceLoadScheduler);

 public:
  // An option to use in calling Request(). If kCanNotBeThrottled is specified,
  // the request should be granted and Run() should be called synchronously.
  // Otherwise, OnRequestGranted() could be called later when other outstanding
  // requests are finished.
  enum class ThrottleOption { kCanBeThrottled, kCanNotBeThrottled };

  // Returned on Request(). Caller should need to return it via Release().
  using RequestId = size_t;

  static constexpr RequestId kInvalidRequestId = 0u;

  static ResourceLoadScheduler* Create() { return new ResourceLoadScheduler(); }
  ~ResourceLoadScheduler() {}
  DEFINE_INLINE_TRACE() {}

  // Makes a request. RequestId should be set before Run() is invoked so that
  // caller can call Release() with the assigned RequestId correctly even if
  // the invocation happens synchronously.
  void Request(ResourceLoadSchedulerClient*, ThrottleOption, RequestId*);

  // ResourceLoadSchedulerClient should call this method when the loading is
  // finished, or canceled.
  bool Release(RequestId);

 private:
  ResourceLoadScheduler();

  // Generates the next RequestId.
  RequestId GenerateRequestId();

  // Picks up one client if there is a budget and route it to run.
  void MaybeRun();

  // Grants a client to run,
  void Run(RequestId, ResourceLoadSchedulerClient*);

  RequestId current_id_;

  // Holds clients that were granted and are running.
  HashSet<RequestId> running_requests_;

  // Holds clients that haven't been granted, and are waiting for a grant.
  HashMap<RequestId, ResourceLoadSchedulerClient*> pending_request_map_;
  Deque<RequestId> pending_request_queue_;
};

}  // namespace blink

#endif
