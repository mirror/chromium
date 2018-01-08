// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_INTERFACE_BROKER_BINDER_HOLDER_H_
#define CONTENT_BROWSER_WEB_INTERFACE_BROKER_BINDER_HOLDER_H_

#include <string>
#include <unordered_map>

#include "base/callback.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace url {
class Origin;
}

namespace content {

class RenderFrameHost;
class RenderProcessHost;
class WebInterfaceFilter;
enum class WebContextType;

class BinderHolder {
 public:
  BinderHolder(
      std::string interface_name,
      std::unique_ptr<WebInterfaceFilter> filter,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderFrameHost*)> frame_binder,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedInterfaceEndpointHandle,
                                   RenderFrameHost*)> associated_frame_binder,
      base::RepeatingCallback<void(const std::string&,
                                   mojo::ScopedMessagePipeHandle,
                                   RenderProcessHost*,
                                   const url::Origin&)> origin_binder);

  BinderHolder(BinderHolder&&);
  BinderHolder& operator=(BinderHolder&&);
  ~BinderHolder();

  void BindInterfaceForWorker(WebContextType,
                              mojo::ScopedMessagePipeHandle interface_pipe,
                              RenderProcessHost* host,
                              const url::Origin& origin,
                              mojo::ReportBadMessageCallback) const;

  void BindInterfaceForFrame(mojo::ScopedMessagePipeHandle interface_pipe,
                             RenderFrameHost* frame,
                             mojo::ReportBadMessageCallback) const;

  void BindAssociatedInterfaceForFrame(
      mojo::ScopedInterfaceEndpointHandle handle,
      RenderFrameHost* frame,
      mojo::ReportBadMessageCallback) const;

 private:
  bool PassesFilter(WebContextType,
                    RenderFrameHost*,
                    RenderProcessHost*,
                    const url::Origin&,
                    mojo::ReportBadMessageCallback) const;

  void ReportBadMessage(mojo::ReportBadMessageCallback) const;

  std::string interface_name_;
  std::unique_ptr<WebInterfaceFilter> filter_;
  base::RepeatingCallback<
      void(const std::string&, mojo::ScopedMessagePipeHandle, RenderFrameHost*)>
      frame_binder_;
  base::RepeatingCallback<void(const std::string&,
                               mojo::ScopedInterfaceEndpointHandle,
                               RenderFrameHost*)>
      associated_frame_binder_;
  base::RepeatingCallback<void(const std::string&,
                               mojo::ScopedMessagePipeHandle,
                               RenderProcessHost*,
                               const url::Origin&)>
      origin_binder_;

  DISALLOW_COPY_AND_ASSIGN(BinderHolder);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_INTERFACE_BROKER_BINDER_HOLDER_H_
