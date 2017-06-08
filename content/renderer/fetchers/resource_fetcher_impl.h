// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_FETCHERS_RESOURCE_FETCHER_IMPL_H_
#define CONTENT_RENDERER_FETCHERS_RESOURCE_FETCHER_IMPL_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "content/public/renderer/resource_fetcher.h"

namespace blink {
class WebLocalFrame;
class WebURLLoader;
class WebURLRequest;
}

namespace content {

class ResourceFetcherImpl : public ResourceFetcher {
 public:
  // ResourceFetcher implementation:
  void Start(blink::WebLocalFrame* frame,
             std::unique_ptr<blink::WebURLRequest> request,
             const Callback& callback) override;
  void SetTimeout(const base::TimeDelta& timeout) override;

 private:
  friend class ResourceFetcher;

  class ClientImpl;

  explicit ResourceFetcherImpl();

  ~ResourceFetcherImpl() override;

  void OnLoadComplete();
  void Cancel();

  std::unique_ptr<blink::WebURLLoader> loader_;
  std::unique_ptr<ClientImpl> client_;

  // Limit how long to wait for the server.
  base::OneShotTimer timeout_timer_;

  DISALLOW_COPY_AND_ASSIGN(ResourceFetcherImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_FETCHERS_RESOURCE_FETCHER_IMPL_H_
