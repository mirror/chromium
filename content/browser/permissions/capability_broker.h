// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PERMISSIONS_CAPABILITY_BROKER_H_
#define CONTENT_BROWSER_PERMISSIONS_CAPABILITY_BROKER_H_

#include <map>
#include <string>

#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/binder_registry.h"

// Capability Broker: vends capability where capabilities are represented by
// mojo interfaces that we expose to requesting render frames, workers, etc,
// based on origin checks, web permissions, etc...

namespace blink {
namespace mojom {
enum class PermissionStatus : int32_t;
}
}  // namespace blink

namespace content {
class RenderFrameHost;
enum class PermissionType;
class PermissionConfiguration;

class CapabilityBroker {
 public:
  static CapabilityBroker* GetInstance();

  void BindInterfaceForFrame(RenderFrameHost* frame,
                             service_manager::BinderRegistry* registry,
                             const std::string& interface_name,
                             mojo::ScopedMessagePipeHandle interface_pipe);

 private:
  CapabilityBroker();
  ~CapabilityBroker();

  void RequestPermissionDone(RenderFrameHost* frame,
                             service_manager::BinderRegistry* registry,
                             const std::string& interface_name,
                             mojo::ScopedMessagePipeHandle interface_pipe,
                             PermissionConfiguration* configuration,
                             blink::mojom::PermissionStatus result);

  void GetInterface(RenderFrameHost* frame,
                    service_manager::BinderRegistry* registry,
                    const std::string& interface_name,
                    mojo::ScopedMessagePipeHandle interface_pipe,
                    PermissionConfiguration* configuration);

  // Maps interface name to PermissionConfiguration.
  std::map<std::string, PermissionConfiguration> permissions_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_PERMISSIONS_CAPABILITY_BROKER_H_
