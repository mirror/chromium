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
class FetchDispatcher final
    : public GarbageCollectedFinalized<FetchDispatcher> {
  WTF_MAKE_NONCOPYABLE(FetchDispatcher);

 public:
  class Client {
   public:
    virtual void OnRequestDispatched() = 0;
  };

  static FetchDispatcher* Create() { return new FetchDispatcher(); }
  ~FetchDispatcher() {}
  DECLARE_VIRTUAL_TRACE();

  void Request(Client*);
  bool Release(Client*);

 private:
  FetchDispatcher();

  HashSet<Client*> running_requests_;
  Deque<Client*> pending_requests_;
};

}  // namespace blink

#endif
