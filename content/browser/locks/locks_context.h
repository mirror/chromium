// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOCKS_LOCK_CONTEXT_H_
#define CONTENT_BROWSER_LOCKS_LOCK_CONTEXT_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/WebKit/public/platform/modules/locks/locks_service.mojom.h"
#include "url/origin.h"

namespace content {

// One instance of this exists per StoragePartition, and services multiple
// child processes/origins. An instance must only be used on the sequence
// it was created on.
class LocksContext : public base::RefCountedThreadSafe<LocksContext>,
                     public blink::mojom::LocksService {
 public:
  // When ReleaseLock() is called, callers may know that more releases are
  // coming that would invalidate other pending requests, so can specify
  // kDoNotProcessRequests for a batch of releases, then call
  // ProcessRequests() directly to process any affected origin.
  enum PostReleaseAction { kDoProcessRequests, kDoNotProcessRequests };

  LocksContext();

  void CreateService(blink::mojom::LocksServiceRequest request);

  // Request a lock. When the lock is acquired, |callback| will be invoked with
  // a LockHandle.
  void RequestLock(const url::Origin& origin,
                   const std::vector<std::string>& scope,
                   LockMode mode,
                   WaitMode wait,
                   blink::mojom::LockRequestPtr request) override;

  // Called by a LockHandle's implementation when destructed.
  void ReleaseLock(const url::Origin& origin,
                   int64_t id,
                   PostReleaseAction = kDoProcessRequests);

  // Called internally when a lock is requested and optionally when a lock
  // is released, to process outstanding requests within the origin.
  void ProcessRequests(const url::Origin& origin);

 protected:
  friend class base::RefCountedThreadSafe<LocksContext>;
  ~LocksContext() override;

 private:
  // Internal representation of a lock request or held lock.
  struct Lock;

  // State for a particular origin.
  struct OriginState {
    OriginState();
    ~OriginState();
    std::map<int64_t, std::unique_ptr<Lock>> requested;
    std::map<int64_t, std::unique_ptr<Lock>> held;
    std::set<std::string> shared;
    std::set<std::string> exclusive;
  };

  bool IsGrantable(const url::Origin& origin,
                   const std::set<std::string>& scope,
                   LockMode mode);

  void DebugDumpState() const;

  mojo::BindingSet<blink::mojom::LocksService> bindings_;

  int64_t next_lock_id = 1;
  std::map<url::Origin, OriginState> origins_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<LocksContext> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LocksContext);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOCKS_LOCK_CONTEXT_H
