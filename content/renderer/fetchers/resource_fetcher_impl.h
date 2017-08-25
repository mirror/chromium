// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_FETCHERS_RESOURCE_FETCHER_IMPL_H_
#define CONTENT_RENDERER_FETCHERS_RESOURCE_FETCHER_IMPL_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/renderer/resource_fetcher.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

struct ResourceRequest;

class ResourceFetcherImpl : public ResourceFetcher {
 public:
  // ResourceFetcher implementation:
  void SetTimeout(const base::TimeDelta& timeout) override;
  void Cancel() override;

 private:
  friend class ResourceFetcher;
  class ClientImpl;

  ResourceFetcherImpl(const ResourceRequest& request,
                      base::SingleThreadTaskRunner* task_runner,
                      Callback callback);
  ~ResourceFetcherImpl() override;

  void OnLoadComplete();

  std::unique_ptr<ClientImpl> client_;

  // Limit how long to wait for the server.
  base::OneShotTimer timeout_timer_;

  DISALLOW_COPY_AND_ASSIGN(ResourceFetcherImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_FETCHERS_RESOURCE_FETCHER_IMPL_H_
