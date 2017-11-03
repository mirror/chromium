// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/proxy_resolver_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace {

void BindConnectorOnUIThread(service_manager::mojom::ConnectorRequest request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(request));
}

}  // namespace

ProxyResolverManager::ProxyResolverManager() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

ProxyResolverManager::~ProxyResolverManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

proxy_resolver::mojom::ProxyResolverFactoryPtr
ProxyResolverManager::CreateWithStrongBinding() {
  proxy_resolver::mojom::ProxyResolverFactoryPtr proxy_resolver_factory;
  mojo::MakeStrongBinding(std::make_unique<ProxyResolverManager>(),
                          mojo::MakeRequest(&proxy_resolver_factory));
  return proxy_resolver_factory;
}

void ProxyResolverManager::CreateResolver(
    const std::string& pac_script,
    proxy_resolver::mojom::ProxyResolverRequest req,
    proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr client) {
  DCHECK(thread_checker_.CalledOnValidThread());

  InitServiceManagerConnector();

  proxy_resolver::mojom::ProxyResolverFactoryPtr resolver_factory;
  service_manager_connector_->BindInterface(
      proxy_resolver::mojom::kProxyResolverServiceName,
      mojo::MakeRequest(&resolver_factory));
  resolver_factory->CreateResolver(pac_script, std::move(req),
                                   std::move(client));
}

void ProxyResolverManager::InitServiceManagerConnector() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (service_manager_connector_)
    return;

  // The existing ServiceManagerConnection retrieved with
  // ServiceManagerConnection::GetForProcess() lives on the UI thread, so we
  // can't access it from here. We create our own connector so it can be used
  // right away and will bind it on the UI thread.
  service_manager::mojom::ConnectorRequest request;
  service_manager_connector_ = service_manager::Connector::Create(&request);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&BindConnectorOnUIThread, base::Passed(&request)));
}
