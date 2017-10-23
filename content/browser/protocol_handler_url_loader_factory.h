// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PROTOCOL_HANDLER_URL_LOADER_FACTORY_H_
#define CONTENT_BROWSER_PROTOCOL_HANDLER_URL_LOADER_FACTORY_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/browser/protocol_handler.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace content {

class RenderProcessHost;
class StoragePartition;

// A URLLoaderFactory implementation used to handle non-network resource
// requests within the browser process. Custom handlers may be registered on a
// per-scheme basis at construction time.
//
// Any instance of this object may handle EITHER navigation requests OR
// subresource requests, but not both.
class CONTENT_EXPORT ProtocolHandlerURLLoaderFactory
    : public mojom::URLLoaderFactory {
 public:
  using ProtocolHandlerMap =
      std::map<std::string, std::unique_ptr<ProtocolHandler>>;

  // Constructs a factory over a custom mapping of protocol handlers. Typically
  // the static CreateFor* methods below should be used.
  explicit ProtocolHandlerURLLoaderFactory(
      ProtocolHandlerMap protocol_handlers);
  ~ProtocolHandlerURLLoaderFactory() override;

  static std::unique_ptr<ProtocolHandlerURLLoaderFactory> CreateForNavigation(
      StoragePartition* partition);

  static std::unique_ptr<ProtocolHandlerURLLoaderFactory> CreateForSubresources(
      RenderProcessHost* process_host);

  void BindRequest(mojom::URLLoaderFactoryRequest request);

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

 private:
  ProtocolHandlerMap protocol_handlers_;
  mojo::BindingSet<mojom::URLLoaderFactory> bindings_;

  DISALLOW_COPY_AND_ASSIGN(ProtocolHandlerURLLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_PROTOCOL_HANDLER_URL_LOADER_FACTORY_H_
