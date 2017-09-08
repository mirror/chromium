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

class FlagsServiceImpl;

// One instance of this exists per StoragePartition, and services multiple
// child processes/origins. All use of this class must be from the Browser's
// IO thread.
class FlagsContext : public base::RefCountedThreadSafe<FlagsContext>,
                     public blink::mojom::FlagsService {
 public:
  FlagsContext();

  // Shutdown must be called before deleting this. Call on UI thread.
  // void Shutdown();

  // Create a FlagsServiceImpl owned by this. Call on UI thread.
  void CreateService(blink::mojom::FlagsServiceRequest request);

  // Request a flag. The id will be returned so that the request
  // can be cancelled or flag released. When the flag is acquired
  // |callback| will be called with the flag id.
  void RequestFlag(
      const url::Origin& origin,
      const std::vector<std::string>& scope,
      blink::mojom::FlagMode mode,
      blink::mojom::FlagsService::RequestFlagCallback callback) override;

  // Called when the holder has released a flag, or when the request is
  // obsolete e.g. due to a terminated context or renderer process.
  void ReleaseFlag(const url::Origin& origin, int64_t id) override;

  // Called internally when a flag is requested and optionally when a flag
  // is released, to process outstanding requests within the origin.
  void ProcessRequests(const url::Origin& origin);

  // Called by FlagsServiceImpl instances so they can be deleted. Service must
  // release all held flags before calling. Call on IO thread.
  // void ServiceHadConnectionError(FlagsServiceImpl* service);

 protected:
  friend class base::RefCountedThreadSafe<FlagsContext>;
  ~FlagsContext() override;

 private:
  // Internal representation of a flag request or held flag.
  struct Flag;

  // void CreateServiceOnIOThread(
  //    blink::mojom::FlagsServiceRequest request);

  // void ShutdownOnIOThread();

  // The services are owned by this. Deleted during ShutdownOnIO or when the
  // channel is closed via ServiceHadConnectionError. Used on IO thread.
  // std::set<FlagsServiceImpl*> services_;

  mojo::BindingSet<blink::mojom::FlagsService> bindings_;

  int64_t next_id_ = 1;
  std::map<url::Origin, std::vector<std::unique_ptr<Flag>>> flags_requested_;
  std::map<url::Origin, std::vector<std::unique_ptr<Flag>>> flags_held_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(FlagsContext);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FLAGS_FLAG_CONTEXT_H
