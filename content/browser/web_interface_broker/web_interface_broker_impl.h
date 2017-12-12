// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_INTERFACE_BROKER_WEB_INTERFACE_BROKER_IMPL_H_
#define CONTENT_BROWSER_WEB_INTERFACE_BROKER_WEB_INTERFACE_BROKER_IMPL_H_

#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "content/public/browser/web_interface_broker.h"
#include "content/public/browser/web_interface_filter.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {

class WebInterfaceBrokerImpl : public WebInterfaceBroker {
 public:
  WebInterfaceBrokerImpl();
  ~WebInterfaceBrokerImpl() override;

  bool TryBindInterface(WebContextType,
                        const std::string& interface_name,
                        mojo::ScopedMessagePipeHandle* interface_pipe,
                        RenderProcessHost* host,
                        const url::Origin& origin,
                        mojo::ReportBadMessageCallback*) override;

  // Try binding an interface request |interface_pipe| for |interface_name|
  // received from |frame|.
  bool TryBindInterface(const std::string& interface_name,
                        mojo::ScopedMessagePipeHandle* interface_pipe,
                        RenderFrameHost* frame,
                        mojo::ReportBadMessageCallback*) override;

 private:
  void AddInterfaceImpl(
      const std::string& interface_name,
      std::unique_ptr<WebInterfaceFilter>,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderFrameHost*)>,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderProcessHost*,
                                   const url::Origin&)>) override;

  void AddWebContentsObserverInterface(
      const std::string& interface_name,
      std::unique_ptr<WebInterfaceFilter> filter) override;

  bool TryBindInterfaceImpl(WebContextType,
                            const std::string& interface_name,
                            mojo::ScopedMessagePipeHandle* interface_pipe,
                            RenderFrameHost*,
                            RenderProcessHost*,
                            const url::Origin&,
                            mojo::ReportBadMessageCallback*);
  service_manager::BinderRegistryWithArgs<RenderProcessHost*,
                                          const url::Origin&>
      binder_registry_;
  service_manager::BinderRegistryWithArgs<RenderFrameHost*>
      frame_binder_registry_;

  std::unordered_map<std::string, std::unique_ptr<WebInterfaceFilter>> filters_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_INTERFACE_BROKER_WEB_INTERFACE_BROKER_IMPL_H_
