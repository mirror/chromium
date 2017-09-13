// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/binding.h"

#if !defined(OS_ANDROID)
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/utility_process_host.h"
#include "content/public/browser/utility_process_host_client.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/l10n/l10n_util.h"
#else  // defined(OS_ANDROID)
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/proxy/mojo_proxy_resolver_factory_impl.h"
#endif  // !defined(OS_ANDROID)

namespace {
int g_utility_process_idle_timeout_seconds = 5;
}

class ChromeMojoProxyResolverFactory::ResolverFactoryChannel
    : public net::interfaces::ProxyResolverFactory {
 public:
  ResolverFactoryChannel(ChromeMojoProxyResolverFactory* parent_factory,
                         net::interfaces::ProxyResolverFactoryRequest request)
      : parent_factory_(parent_factory), binding_(this, std::move(request)) {
    binding_.set_connection_error_handler(base::BindOnce(
        &ResolverFactoryChannel::OnConnectionError, base::Unretained(this)));
  }

  ~ResolverFactoryChannel() override {
    if (num_proxy_resolvers_ > 0)
      parent_factory_->OnNumProxyResolversChanged(-num_proxy_resolvers_);
  }

  int num_proxy_resolvers() const { return num_proxy_resolvers_; }

 private:
  // net::interfaces::ProxyResolverFactory implementation:

  void CreateResolver(
      const std::string& pac_script,
      mojo::InterfaceRequest<net::interfaces::ProxyResolver> req,
      net::interfaces::ProxyResolverFactoryRequestClientPtr client) override {
    // Always increment number of resolvers, even on errors. The caller will
    // inform this class of any connection errors, including those that happen
    // when utility process creation fails.
    num_proxy_resolvers_++;
    parent_factory_->OnNumProxyResolversChanged(1);

    net::interfaces::ProxyResolverFactory* resolver_factory =
        parent_factory_->ResolverFactory();
    if (!resolver_factory) {
      // If factory creation failed, just destroy |req|, which should cause a
      // connection error on the other end.
      return;
    }

    resolver_factory->CreateResolver(pac_script, std::move(req),
                                     std::move(client));
  }

  void OnResolverDestroyed() override {
    // If this happens, the count get messed up, but at worse, this means either
    // the utility process is killed when it shouldn't be, or is not killed
    // when it should be, neither of which is too concerning from a security
    // standpoint.
    DCHECK_LE(0, num_proxy_resolvers_);

    num_proxy_resolvers_--;
    parent_factory_->OnNumProxyResolversChanged(-1);
    // Can't call OnResolverDestroyed() on the utility process's
    // ProxyResolverFactory - if the utility processes crashes and is restarted,
    // it's unclear if this count is for the old or new utility process. This
    // could be worked around, but since the utility process's factory doesn't
    // care, doesn't seem worth the effort.
  }

  void OnConnectionError() { parent_factory_->OnConnectionClosed(this); }

  ChromeMojoProxyResolverFactory* parent_factory_;
  mojo::Binding<net::interfaces::ProxyResolverFactory> binding_;
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

// static
void ChromeMojoProxyResolverFactory::SetUtilityProcessIdleTimeoutSecsForTesting(
    int seconds) {
  g_utility_process_idle_timeout_seconds = seconds;
}

net::interfaces::ProxyResolverFactoryPtr
ChromeMojoProxyResolverFactory::CreateFactoryInterface() {
  net::interfaces::ProxyResolverFactoryPtr factory;
  channels_.insert(std::make_unique<ResolverFactoryChannel>(
      this, mojo::MakeRequest(&factory)));
  return factory;
}

ChromeMojoProxyResolverFactory::ChromeMojoProxyResolverFactory() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

ChromeMojoProxyResolverFactory::~ChromeMojoProxyResolverFactory() {
  DCHECK(thread_checker_.CalledOnValidThread());
  channels_.clear();
}

net::interfaces::ProxyResolverFactory*
ChromeMojoProxyResolverFactory::ResolverFactory() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (utility_process_resolver_factory_)
    return utility_process_resolver_factory_.get();

#if defined(OS_ANDROID)
  mojo::MakeStrongBinding(
      base::MakeUnique<net::MojoProxyResolverFactoryImpl>(),
      mojo::MakeRequest(&utility_process_resolver_factory_));
#else   // !defined(OS_ANDROID)
  DCHECK(!weak_utility_process_host_);

  DVLOG(1) << "Attempting to create utility process for proxy resolver";
  content::UtilityProcessHost* utility_process_host =
      content::UtilityProcessHost::Create(
          scoped_refptr<content::UtilityProcessHostClient>(),
          base::ThreadTaskRunnerHandle::Get());
  utility_process_host->SetName(
      l10n_util::GetStringUTF16(IDS_UTILITY_PROCESS_PROXY_RESOLVER_NAME));
  bool process_started = utility_process_host->Start();
  if (process_started) {
    BindInterface(utility_process_host, &utility_process_resolver_factory_);
    weak_utility_process_host_ = utility_process_host->AsWeakPtr();
  } else {
    LOG(ERROR) << "Unable to connect to utility process";
    return nullptr;
  }
#endif  // defined(OS_ANDROID)

  utility_process_resolver_factory_.set_connection_error_handler(base::Bind(
      &ChromeMojoProxyResolverFactory::DestroyFactory, base::Unretained(this)));
  return utility_process_resolver_factory_.get();
}

void ChromeMojoProxyResolverFactory::DestroyFactory() {
  utility_process_resolver_factory_.reset();
#if !defined(OS_ANDROID)
  delete weak_utility_process_host_.get();
  weak_utility_process_host_.reset();
#endif
}

void ChromeMojoProxyResolverFactory::OnIdleTimeout() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(num_proxy_resolvers_, 0);
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

void ChromeMojoProxyResolverFactory::OnNumProxyResolversChanged(int delta) {
  num_proxy_resolvers_ += delta;
  DCHECK_GE(num_proxy_resolvers_, 0);

#if DCHECK_IS_ON()
  int real_num_proxy_resolvers = 0;
  for (const auto& binding : channels_) {
    real_num_proxy_resolvers += binding->num_proxy_resolvers();
  }
  DCHECK_EQ(real_num_proxy_resolvers, num_proxy_resolvers_);
#endif  // DCHECK_IS_ON()

  if (num_proxy_resolvers_ == 0) {
    idle_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromSeconds(g_utility_process_idle_timeout_seconds),
        this, &ChromeMojoProxyResolverFactory::OnIdleTimeout);
  } else {
    idle_timer_.Stop();
  }
}
