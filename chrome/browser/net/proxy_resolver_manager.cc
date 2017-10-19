// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/proxy_resolver_manager.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"

namespace {

constexpr base::TimeDelta kUtilityProcessIdleTimeout =
    base::TimeDelta::FromSeconds(5);

void BindConnectorOnUIThread(service_manager::mojom::ConnectorRequest request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(request));
}

}  // namespace

// ProxyResolverFactory proxy class that tracks the number of ProxyResolvers it
// creates. Need a separate class for each factory to know how many
// ProxyResolvers were in use in the case of a network process crash.
class ProxyResolverManager::ResolverFactoryChannel
    : public proxy_resolver::mojom::ProxyResolverFactory {
 public:
  ResolverFactoryChannel(
      ProxyResolverManager* parent_factory,
      proxy_resolver::mojom::ProxyResolverFactoryRequest request)
      : parent_factory_(parent_factory), binding_(this, std::move(request)) {
    binding_.set_connection_error_handler(base::BindOnce(
        &ResolverFactoryChannel::OnConnectionError, base::Unretained(this)));
  }

  ~ResolverFactoryChannel() override {
    if (has_proxy_resolvers())
      parent_factory_->OnNumProxyResolversChanged();
  }

  bool has_proxy_resolvers() const { return num_proxy_resolvers_ > 0; }

 private:
  // net::interfaces::ProxyResolverFactory implementation:

  void CreateResolver(
      const std::string& pac_script,
      proxy_resolver::mojom::ProxyResolverRequest req,
      proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr client)
      override {
    // Always increment number of resolvers, even on errors. The caller will
    // inform this class of any connection errors, including those that happen
    // when utility process creation fails.
    num_proxy_resolvers_++;
    parent_factory_->OnNumProxyResolversChanged();

    if (!parent_factory_->ResolverFactory()) {
      // If factory creation failed, just destroy |req|, which should cause a
      // connection error on the other end.
      return;
    }

    parent_factory_->ResolverFactory()->CreateResolver(
        pac_script, std::move(req), std::move(client));
  }

  void OnResolverDestroyed() override {
    // Can't call OnResolverDestroyed() on the utility process's
    // ProxyResolverFactory - if the utility processes crashes and is restarted,
    // it's unclear if this count is for the old or new utility process. This
    // could be worked around, but since the utility process's factory doesn't
    // care, doesn't seem worth the effort.

    // Don't bother protecting this from becoming less than 0 - at worse,
    // underflow here can keep the service process alive, but just keeping
    // around a ProxyResolver will do the same thing.
    num_proxy_resolvers_--;
    parent_factory_->OnNumProxyResolversChanged();
  }

  void OnConnectionError() { parent_factory_->OnConnectionClosed(this); }

  ProxyResolverManager* parent_factory_;
  mojo::Binding<proxy_resolver::mojom::ProxyResolverFactory> binding_;
  int num_proxy_resolvers_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ResolverFactoryChannel);
};

// static
ProxyResolverManager* ProxyResolverManager::GetInstance() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  return base::Singleton<
      ProxyResolverManager,
      base::LeakySingletonTraits<ProxyResolverManager>>::get();
}

proxy_resolver::mojom::ProxyResolverFactoryPtr
ProxyResolverManager::CreateFactoryInterface() {
  proxy_resolver::mojom::ProxyResolverFactoryPtr factory;
  channels_.insert(std::make_unique<ResolverFactoryChannel>(
      this, mojo::MakeRequest(&factory)));
  return factory;
}

ProxyResolverManager::ProxyResolverManager()
    : factory_idle_timeout_(kUtilityProcessIdleTimeout) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

ProxyResolverManager::~ProxyResolverManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void ProxyResolverManager::SetFactoryIdleTimeoutForTests(
    const base::TimeDelta& timeout) {
  factory_idle_timeout_ = timeout;
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

proxy_resolver::mojom::ProxyResolverFactory*
ProxyResolverManager::ResolverFactory() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!resolver_factory_) {
    InitServiceManagerConnector();

    service_manager_connector_->BindInterface(
        proxy_resolver::mojom::kProxyResolverServiceName,
        mojo::MakeRequest(&resolver_factory_));

    resolver_factory_.set_connection_error_handler(base::Bind(
        &ProxyResolverManager::DestroyFactory, base::Unretained(this)));
  }
  return resolver_factory_.get();
}

void ProxyResolverManager::DestroyFactory() {
  resolver_factory_.reset();
}

void ProxyResolverManager::OnIdleTimeout() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DestroyFactory();
}

void ProxyResolverManager::OnConnectionClosed(ResolverFactoryChannel* binding) {
  auto it = std::find_if(
      channels_.begin(), channels_.end(),
      [binding](
          const std::unique_ptr<ResolverFactoryChannel>& factory_binding) {
        return factory_binding.get() == binding;
      });
  DCHECK(it != channels_.end());
  channels_.erase(it);
}

void ProxyResolverManager::OnNumProxyResolversChanged() {
  bool has_proxy_resolvers = false;
  for (const auto& binding : channels_) {
    if (binding->has_proxy_resolvers()) {
      has_proxy_resolvers = true;
      break;
    }
  }

  if (!has_proxy_resolvers) {
    idle_timer_.Start(FROM_HERE, factory_idle_timeout_, this,
                      &ProxyResolverManager::OnIdleTimeout);
  } else {
    idle_timer_.Stop();
  }
}
