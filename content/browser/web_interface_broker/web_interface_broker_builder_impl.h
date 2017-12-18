// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_INTERFACE_BROKER_WEB_INTERFACE_BROKER_BUILDER_IMPL_H_
#define CONTENT_BROWSER_WEB_INTERFACE_BROKER_WEB_INTERFACE_BROKER_BUILDER_IMPL_H_

#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "content/public/browser/web_interface_broker_builder.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {
class WebInterfaceFilter;

class WebInterfaceBrokerBuilderImpl : public WebInterfaceBrokerBuilder {
 public:
  WebInterfaceBrokerBuilderImpl(WorkerDelegate worker_delegate,
                                FrameDelegate frame_delegate);
  ~WebInterfaceBrokerBuilderImpl() override;

  std::unique_ptr<WebInterfaceBroker> Build() override;

 private:
  void AddInterfaceImpl(
      base::StringPiece interface_name,
      std::unique_ptr<WebInterfaceFilter>,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderFrameHost*)>,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderProcessHost*,
                                   const url::Origin&)>) override;

  void AddWebContentsObserverInterfaceImpl(
      base::StringPiece interface_name,
      std::unique_ptr<WebInterfaceFilter> filter) override;

  void ForwardInterfaceToService(base::StringPiece interface_name,
                                 std::unique_ptr<WebInterfaceFilter> filter,
                                 base::StringPiece service_name) override;

  std::unique_ptr<service_manager::BinderRegistryWithArgs<RenderProcessHost*,
                                                          const url::Origin&>>
      binder_registry_;
  std::unique_ptr<service_manager::BinderRegistryWithArgs<RenderFrameHost*>>
      frame_binder_registry_;

  std::unordered_map<std::string, std::unique_ptr<WebInterfaceFilter>> filters_;

  WorkerDelegate worker_delegate_;
  FrameDelegate frame_delegate_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_INTERFACE_BROKER_WEB_INTERFACE_BROKER_BUILDER_IMPL_H_
