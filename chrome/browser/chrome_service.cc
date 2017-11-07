// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_service.h"

#include "components/spellcheck/spellcheck_build_features.h"
#include "components/startup_metric_utils/browser/startup_metric_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/launchable.h"
#if defined(USE_OZONE)
#include "services/ui/public/cpp/input_devices/input_device_controller.h"
#endif
#endif
#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "chrome/browser/spellchecker/spell_check_host_impl.h"
#if BUILDFLAG(HAS_SPELLCHECK_PANEL)
#include "chrome/browser/spellchecker/spell_check_panel_host_impl.h"
#endif
#endif

class ChromeService::IOThreadContext : public service_manager::Service {
 public:
  IOThreadContext() {
#if defined(OS_CHROMEOS)
#if defined(USE_OZONE)
    input_device_controller_.AddInterface(&registry_);
#endif
    registry_.AddInterface(base::Bind(&chromeos::Launchable::Bind,
                                      base::Unretained(&launchable_)));
#endif
    registry_.AddInterface(
        base::Bind(&startup_metric_utils::StartupMetricHostImpl::Create));
#if BUILDFLAG(ENABLE_SPELLCHECK)
    registry_with_source_info_.AddInterface(
        base::Bind(&SpellCheckHostImpl::Create),
        content::BrowserThread::GetTaskRunnerForThread(
            content::BrowserThread::UI));
#if BUILDFLAG(HAS_SPELLCHECK_PANEL)
    registry_.AddInterface(base::Bind(&SpellCheckPanelHostImpl::Create),
                           content::BrowserThread::GetTaskRunnerForThread(
                               content::BrowserThread::UI));
#endif
#endif
  }
  ~IOThreadContext() override = default;

  void BindInterface(const service_manager::Identity& identity,
                     const std::string& interface_name,
                     mojo::ScopedMessagePipeHandle interface_pipe) {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(&ChromeService::IOThreadContext::BindInterfaceOnIOThread,
                   base::Unretained(this), identity, interface_name,
                   base::Passed(&interface_pipe)));
  }

 private:
  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& remote_info,
                       const std::string& name,
                       mojo::ScopedMessagePipeHandle handle) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    content::OverrideOnBindInterface(remote_info, name, &handle);
    if (!handle.is_valid())
      return;

    if (!registry_.TryBindInterface(name, &handle))
      registry_with_source_info_.TryBindInterface(name, &handle, remote_info);
  }

  void BindInterfaceOnIOThread(const service_manager::Identity& identity,
                               const std::string& interface_name,
                               mojo::ScopedMessagePipeHandle interface_pipe) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    context()->connector()->BindInterface(identity, interface_name,
                                          std::move(interface_pipe));
  }

  service_manager::BinderRegistry registry_;
  service_manager::BinderRegistryWithArgs<
      const service_manager::BindSourceInfo&>
      registry_with_source_info_;

#if defined(OS_CHROMEOS)
  chromeos::Launchable launchable_;
#if defined(USE_OZONE)
  ui::InputDeviceController input_device_controller_;
#endif
#endif

  DISALLOW_COPY_AND_ASSIGN(IOThreadContext);
};

ChromeService::ChromeService()
    : io_thread_context_(std::make_unique<IOThreadContext>()) {}
ChromeService::~ChromeService() {}

service_manager::EmbeddedServiceInfo::ServiceFactory
ChromeService::CreateChromeServiceFactory() {
  return base::Bind(&ChromeService::CreateChromeServiceWrapper,
                    base::Unretained(this));
}

void ChromeService::BindInterface(
    const service_manager::Identity& identity,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  io_thread_context_->BindInterface(identity, interface_name,
                                    std::move(interface_pipe));
}

std::unique_ptr<service_manager::Service>
ChromeService::CreateChromeServiceWrapper() {
  return std::make_unique<service_manager::ForwardingService>(
      io_thread_context_.get());
}
