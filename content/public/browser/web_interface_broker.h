// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_BROKER_H_
#define CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_BROKER_H_

#include <string>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace url {
class Origin;
}

namespace content {

class RenderFrameHost;
class RenderProcessHost;
enum class WebContextType;

class CONTENT_EXPORT WebInterfaceBroker {
 public:
  WebInterfaceBroker() = default;
  virtual ~WebInterfaceBroker() = default;

  // Try binding an interface request |interface_pipe| for |interface_name|
  // received from |frame|.
  virtual void BindInterfaceForWorker(
      WebContextType,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle interface_pipe,
      RenderProcessHost* process_host,
      const url::Origin& origin,
      mojo::ReportBadMessageCallback) = 0;

  // Try binding an interface request |interface_pipe| for |interface_name|
  // received from |frame|.
  virtual void BindInterfaceForFrame(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle interface_pipe,
      RenderFrameHost* frame,
      mojo::ReportBadMessageCallback) = 0;

  DISALLOW_COPY_AND_ASSIGN(WebInterfaceBroker);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_WEB_INTERFACE_BROKER_H_
