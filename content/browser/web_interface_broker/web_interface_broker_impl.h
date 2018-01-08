// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_INTERFACE_BROKER_WEB_INTERFACE_BROKER_IMPL_H_
#define CONTENT_BROWSER_WEB_INTERFACE_BROKER_WEB_INTERFACE_BROKER_IMPL_H_

#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "content/public/browser/web_interface_broker.h"
#include "content/public/browser/web_interface_broker_builder.h"

namespace content {
class BinderHolder;

class WebInterfaceBrokerImpl : public WebInterfaceBroker {
 public:
  WebInterfaceBrokerImpl(
      WebInterfaceBrokerBuilder::WorkerDelegate worker_delegate,
      WebInterfaceBrokerBuilder::FrameDelegate frame_delegate,
      WebInterfaceBrokerBuilder::AssociatedFrameDelegate
          associated_frame_delegate,
      std::unordered_map<std::string, BinderHolder> binders);
  ~WebInterfaceBrokerImpl() override;

  void BindInterfaceForWorker(WebContextType,
                              const std::string& interface_name,
                              mojo::ScopedMessagePipeHandle interface_pipe,
                              RenderProcessHost* host,
                              const url::Origin& origin,
                              mojo::ReportBadMessageCallback) override;

  // Try binding an interface request |interface_pipe| for |interface_name|
  // received from |frame|.
  void BindInterfaceForFrame(const std::string& interface_name,
                             mojo::ScopedMessagePipeHandle interface_pipe,
                             RenderFrameHost* frame,
                             mojo::ReportBadMessageCallback) override;

  void BindAssociatedInterfaceForFrame(
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle handle,
      RenderFrameHost* frame,
      mojo::ReportBadMessageCallback) override;

 private:
  const BinderHolder* GetBinder(const std::string& interface_name);

  const std::unordered_map<std::string, BinderHolder> binders_;

  const WebInterfaceBrokerBuilder::WorkerDelegate worker_delegate_;
  const WebInterfaceBrokerBuilder::FrameDelegate frame_delegate_;
  const WebInterfaceBrokerBuilder::AssociatedFrameDelegate
      associated_frame_delegate_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_INTERFACE_BROKER_WEB_INTERFACE_BROKER_IMPL_H_
