// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FLAGS_FLAG_CONTEXT_H_
#define CONTENT_BROWSER_FLAGS_FLAG_CONTEXT_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/WebKit/public/platform/modules/flags/flags_service.mojom.h"
#include "url/origin.h"

namespace content {

// One instance of this exists per StoragePartition, and services multiple
// child processes/origins. An instance must only be used on the sequence
// it was created on.
class FlagsContext : public base::RefCountedThreadSafe<FlagsContext>,
                     public blink::mojom::FlagsService {
 public:
  // When ReleaseFlag() is called, callers may know that more releases are
  // coming that would invalidate other pending requests, so can specify
  // kDoNotProcessRequests for a batch of releases, then call
  // ProcessRequests() directly to process any affected origin.
  enum PostReleaseAction { kDoProcessRequests, kDoNotProcessRequests };

  FlagsContext();

  void CreateService(blink::mojom::FlagsServiceRequest request);

  // Request a flag. When the flag is acquired, |callback| will be invoked with
  // a FlagHandle.
  void RequestFlag(const url::Origin& origin,
                   const std::vector<std::string>& scope,
                   blink::mojom::FlagMode mode,
                   blink::mojom::FlagRequestPtr request) override;

  // Called by a FlagHandle's implementation when destructed.
  void ReleaseFlag(const url::Origin& origin,
                   int64_t id,
                   PostReleaseAction = kDoProcessRequests);

  // Called internally when a flag is requested and optionally when a flag
  // is released, to process outstanding requests within the origin.
  void ProcessRequests(const url::Origin& origin);

 protected:
  friend class base::RefCountedThreadSafe<FlagsContext>;
  ~FlagsContext() override;

 private:
  // Internal representation of a flag request or held flag.
  struct Flag;

  // State for a particular origin.
  struct OriginState {
    OriginState();
    ~OriginState();
    std::map<int64_t, std::unique_ptr<Flag>> requested;
    std::map<int64_t, std::unique_ptr<Flag>> held;
  };

  void DebugDumpState() const;

  mojo::BindingSet<blink::mojom::FlagsService> bindings_;

  int64_t next_id_ = 1;
  std::map<url::Origin, OriginState> origins_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<FlagsContext> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FlagsContext);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FLAGS_FLAG_CONTEXT_H
