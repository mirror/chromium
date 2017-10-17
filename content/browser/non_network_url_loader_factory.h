// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NON_NETWORK_URL_LOADER_FACTORY_H_
#define CONTENT_BROWSER_NON_NETWORK_URL_LOADER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/non_network_protocol_handler.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace content {

// A URLLoaderFactory implementation used to handle non-network resource
// requests within the browser process. Custom handlers may be registered on a
// per-scheme basis via a ProtocolHandlerMap at creation time.
class CONTENT_EXPORT NonNetworkURLLoaderFactory
    : public base::RefCountedThreadSafe<NonNetworkURLLoaderFactory,
                                        BrowserThread::DeleteOnIOThread>,
      public mojom::URLLoaderFactory {
 public:
  explicit NonNetworkURLLoaderFactory(
      NonNetworkProtocolHandlerMap protocol_handlers);

  void BindRequest(mojom::URLLoaderFactoryRequest request);

  // Explicit shutdown must be initiated at an appropriate time such that
  // necessary cleanup work can be done on the IO thread.
  void ShutDown();

 private:
  friend class base::DeleteHelper<NonNetworkURLLoaderFactory>;
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;

  ~NonNetworkURLLoaderFactory() override;

  // mojom::URLLoaderFactory:
  void CreateLoaderAndStart(mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojom::URLLoaderFactoryRequest request) override;

  void BindOnIO(mojom::URLLoaderFactoryRequest request);
  void ShutDownOnIO();

  // After construction, fields below must only be accessed on the IO thread.
  NonNetworkProtocolHandlerMap protocol_handlers_;
  bool shutting_down_ = false;
  mojo::BindingSet<mojom::URLLoaderFactory> bindings_;

  DISALLOW_COPY_AND_ASSIGN(NonNetworkURLLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_NON_NETWORK_URL_LOADER_FACTORY_H_
