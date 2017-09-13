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

// static
ChromeMojoProxyResolverFactory* ChromeMojoProxyResolverFactory::GetInstance() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  return base::Singleton<
      ChromeMojoProxyResolverFactory,
      base::LeakySingletonTraits<ChromeMojoProxyResolverFactory>>::get();
}

void ChromeMojoProxyResolverFactory::BindRequest(
    net::interfaces::ProxyResolverFactoryRequest factory_request) {
  binding_set_.AddBinding(this, std::move(factory_request));
}

ChromeMojoProxyResolverFactory::ChromeMojoProxyResolverFactory() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

ChromeMojoProxyResolverFactory::~ChromeMojoProxyResolverFactory() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void ChromeMojoProxyResolverFactory::CreateFactory() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!resolver_factory_);

#if defined(OS_ANDROID)
  mojo::MakeStrongBinding(base::MakeUnique<net::MojoProxyResolverFactoryImpl>(),
                          mojo::MakeRequest(&resolver_factory_));
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
    BindInterface(utility_process_host, &resolver_factory_);
    weak_utility_process_host_ = utility_process_host->AsWeakPtr();
  } else {
    LOG(ERROR) << "Unable to connect to utility process";
  }
#endif  // defined(OS_ANDROID)
}

void ChromeMojoProxyResolverFactory::CreateResolver(
    const std::string& pac_script,
    mojo::InterfaceRequest<net::interfaces::ProxyResolver> req,
    net::interfaces::ProxyResolverFactoryRequestClientPtr client) {
  if (!resolver_factory_) {
    CreateFactory();
    // If factory creation failed, just destroy |req|, which should cause a
    // connection error on the other end.
    if (!resolver_factory_)
      return;
  }

  resolver_factory_->CreateResolver(pac_script, std::move(req),
                                    std::move(client));
}
