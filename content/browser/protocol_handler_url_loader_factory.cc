// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/protocol_handler_url_loader_factory.h"

#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/task_scheduler/post_task.h"
#include "content/browser/file_protocol_handler_network_service.h"
#include "content/browser/storage_partition_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "url/gurl.h"

namespace content {

namespace {

void RegisterFileProtocolHandler(
    BrowserContext* browser_context,
    ProtocolHandlerURLLoaderFactory::ProtocolHandlerMap* handlers) {
  handlers->emplace(url::kFileScheme,
                    std::make_unique<FileProtocolHandlerNetworkService>(
                        browser_context->GetPath(),
                        base::CreateSequencedTaskRunnerWithTraits(
                            {base::MayBlock(), base::TaskPriority::BACKGROUND,
                             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})));
}

}  // namespace

ProtocolHandlerURLLoaderFactory::ProtocolHandlerURLLoaderFactory(
    ProtocolHandlerMap protocol_handlers)
    : protocol_handlers_(std::move(protocol_handlers)) {}

ProtocolHandlerURLLoaderFactory::~ProtocolHandlerURLLoaderFactory() = default;

// static
std::unique_ptr<ProtocolHandlerURLLoaderFactory>
ProtocolHandlerURLLoaderFactory::CreateForNavigation(
    StoragePartition* partition) {
  BrowserContext* browser_context =
      static_cast<StoragePartitionImpl*>(partition)->browser_context();

  ProtocolHandlerMap handlers;
  RegisterFileProtocolHandler(browser_context, &handlers);
  return std::make_unique<ProtocolHandlerURLLoaderFactory>(std::move(handlers));
}

// static
std::unique_ptr<ProtocolHandlerURLLoaderFactory>
ProtocolHandlerURLLoaderFactory::CreateForSubresources(
    RenderProcessHost* process_host) {
  ProtocolHandlerMap handlers;
  if (!std::string().empty() /* TODO(XXX): What's the correct check here? */) {
    // Only allow file:// resources to be fetched by render processes hosting
    // other file:// resources.
    BrowserContext* browser_context = process_host->GetBrowserContext();
    RegisterFileProtocolHandler(browser_context, &handlers);
  }
  return std::make_unique<ProtocolHandlerURLLoaderFactory>(std::move(handlers));
}

void ProtocolHandlerURLLoaderFactory::BindRequest(
    mojom::URLLoaderFactoryRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ProtocolHandlerURLLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest loader,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  std::string scheme = request.url.scheme();
  auto it = protocol_handlers_.find(scheme);
  if (it == protocol_handlers_.end()) {
    DLOG(ERROR) << "No registered handler for '" << scheme << "' scheme of "
                << "request for " << request.url.spec();
    return;
  }

  it->second->CreateAndStartLoader(request, std::move(loader),
                                   std::move(client));
}

void ProtocolHandlerURLLoaderFactory::Clone(
    mojom::URLLoaderFactoryRequest request) {
  BindRequest(std::move(request));
}

}  // namespace content
