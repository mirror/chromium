// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"

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
class ChromeMojoProxyResolverFactory::ResolverFactoryChannel
    : public proxy_resolver::mojom::ProxyResolverFactory {
 public:
  ResolverFactoryChannel(
      ChromeMojoProxyResolverFactory* parent_factory,
      proxy_resolver::mojom::ProxyResolverFactoryRequest request)
      : parent_factory_(parent_factory), binding_(this, std::move(request)) {
    binding_.set_connection_error_handler(base::BindOnce(
        &ResolverFactoryChannel::OnConnectionError, base::Unretained(this)));
  }

  ~ResolverFactoryChannel() override {
    if (num_proxy_resolvers_ > 0)
      parent_factory_->OnNumProxyResolversChanged();
  }

  int num_proxy_resolvers() const { return num_proxy_resolvers_; }

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
    // If more resolvers are destroyed than were created, consider it a
    // connection error.
    if (num_proxy_resolvers_ == 0) {
      binding_.Unbind();
      OnConnectionError();
      return;
    }

    // Can't call OnResolverDestroyed() on the utility process's
    // ProxyResolverFactory - if the utility processes crashes and is restarted,
    // it's unclear if this count is for the old or new utility process. This
    // could be worked around, but since the utility process's factory doesn't
    // care, doesn't seem worth the effort.

    num_proxy_resolvers_--;
    parent_factory_->OnNumProxyResolversChanged();
  }

  void OnConnectionError() { parent_factory_->OnConnectionClosed(this); }

  ChromeMojoProxyResolverFactory* parent_factory_;
  mojo::Binding<proxy_resolver::mojom::ProxyResolverFactory> binding_;
  int num_proxy_resolvers_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ResolverFactoryChannel);
};

// static
ChromeMojoProxyResolverFactory* ChromeMojoProxyResolverFactory::GetInstance() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  return base::Singleton<
      ChromeMojoProxyResolverFactory,
      base::LeakySingletonTraits<ChromeMojoProxyResolverFactory>>::get();
}

proxy_resolver::mojom::ProxyResolverFactoryPtr
ChromeMojoProxyResolverFactory::CreateFactoryInterface() {
  proxy_resolver::mojom::ProxyResolverFactoryPtr factory;
  channels_.insert(std::make_unique<ResolverFactoryChannel>(
      this, mojo::MakeRequest(&factory)));
  return factory;
}

ChromeMojoProxyResolverFactory::ChromeMojoProxyResolverFactory()
    : factory_idle_timeout_(kUtilityProcessIdleTimeout) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

ChromeMojoProxyResolverFactory::~ChromeMojoProxyResolverFactory() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void ChromeMojoProxyResolverFactory::SetFactoryIdleTimeoutForTests(
    const base::TimeDelta& timeout) {
  factory_idle_timeout_ = timeout;
}

void ChromeMojoProxyResolverFactory::InitServiceManagerConnector() {
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
ChromeMojoProxyResolverFactory::ResolverFactory() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!resolver_factory_) {
    InitServiceManagerConnector();

    service_manager_connector_->BindInterface(
        proxy_resolver::mojom::kProxyResolverServiceName,
        mojo::MakeRequest(&resolver_factory_));

    resolver_factory_.set_connection_error_handler(
        base::Bind(&ChromeMojoProxyResolverFactory::DestroyFactory,
                   base::Unretained(this)));
  }
  return resolver_factory_.get();
}

void ChromeMojoProxyResolverFactory::DestroyFactory() {
  resolver_factory_.reset();
}

void ChromeMojoProxyResolverFactory::OnIdleTimeout() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DestroyFactory();
}

void ChromeMojoProxyResolverFactory::OnConnectionClosed(
    ResolverFactoryChannel* binding) {
  auto it = std::find_if(
      channels_.begin(), channels_.end(),
      [binding](
          const std::unique_ptr<ResolverFactoryChannel>& factory_binding) {
        return factory_binding.get() == binding;
      });
  DCHECK(it != channels_.end());
  channels_.erase(it);
}

void ChromeMojoProxyResolverFactory::OnNumProxyResolversChanged() {
  int num_proxy_resolvers = 0;
  for (const auto& binding : channels_) {
    num_proxy_resolvers += binding->num_proxy_resolvers();
  }

  if (num_proxy_resolvers == 0) {
    idle_timer_.Start(FROM_HERE, factory_idle_timeout_, this,
                      &ChromeMojoProxyResolverFactory::OnIdleTimeout);
  } else {
    idle_timer_.Stop();
  }
}
