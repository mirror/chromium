// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ResourceLoadScheduler_h
#define ResourceLoadScheduler_h

#include "platform/heap/GarbageCollected.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/loader/fetch/Resource.h"

namespace blink {

// Client interface to use the arbitration functionality that
// ResourceLoadScheduler provides.
class PLATFORM_EXPORT ResourceLoadSchedulerClient
    : public GarbageCollectedFinalized<ResourceLoadSchedulerClient> {
 public:
  virtual ~ResourceLoadSchedulerClient() {}

  // Called when the request is ready to go.
  virtual void OnRequestGranted() = 0;

  DEFINE_INLINE_VIRTUAL_TRACE() {}
};

// The ResourceLoadScheduler provides an unified infrastructure to give the
// loader more controls to organize loading requests. The ResourceFetcher
// owns a ResourceLoadScheduler instance to control loading requests that
// the ResourceFetcher instance manages per-context basis.
class ResourceLoadScheduler final
    : public GarbageCollectedFinalized<ResourceLoadScheduler> {
  WTF_MAKE_NONCOPYABLE(ResourceLoadScheduler);

 public:
  // An option to use in calling Request(). If kCanNotBeThrottled is specified,
  // the request should be granted and OnRequestGranted() should be called
  // synchronously. Otherwise, OnRequestGranted() could be called later when
  // other outstanding requests are finished.
  enum class ThrottleOption { kCanBeThrottled, kCanNotBeThrottled };

  static ResourceLoadScheduler* Create() { return new ResourceLoadScheduler(); }
  ~ResourceLoadScheduler() {}
  DECLARE_VIRTUAL_TRACE();

  // Requests to go. Once the caller makes a request, it should call Release()
  // when the process is finished or aborted.
  void Request(ResourceLoadSchedulerClient*, ThrottleOption);

  // ResourceLoadSchedulerClient should call this method when the granted
  // running process is finished, or the requesting or running process is
  // canceled.
  // Returns true if the client is already granted to run. Otherwise it
  // returns false.
  bool Release(ResourceLoadSchedulerClient*);

 private:
  ResourceLoadScheduler();

  // Picks up one client if there is a budget and route it to be granted.
  void Wakeup();

  // Grants a client to run a process.
  void Grant(ResourceLoadSchedulerClient*);

  // Holds clients that were granted and are running.
  HeapHashSet<WeakMember<ResourceLoadSchedulerClient>> running_requests_;

  // Holds clients that haven't been granted, and are waiting for a grant.
  HeapLinkedHashSet<WeakMember<ResourceLoadSchedulerClient>> pending_requests_;
};

}  // namespace blink

#endif
