// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/permissions/capability_broker.h"

#include "base/optional.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_client.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/identity.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"

namespace {

service_manager::Connector* GetConnectorForFrameHost(
    content::RenderFrameHost* frame) {
  content::BrowserContext* context = frame->GetProcess()->GetBrowserContext();
  return content::BrowserContext::GetConnectorFor(context);
}

content::WebContentsImpl* GetDelegateForFrameHost(
    content::RenderFrameHost* frame) {
  return static_cast<content::WebContentsImpl*>(
      content::WebContents::FromRenderFrameHost(frame));
}

}  // namespace

namespace content {

// static
CapabilityBroker* CapabilityBroker::GetInstance() {
  CR_DEFINE_STATIC_LOCAL(CapabilityBroker, broker_instance, ());
  return &broker_instance;
}

class PermissionConfiguration {
 public:
  PermissionConfiguration(base::Optional<PermissionType> permission,
                          const std::string& service)
      : permission_(permission), service_(service) {}

  ~PermissionConfiguration() = default;

  PermissionConfiguration(PermissionConfiguration&& other) = default;
  PermissionConfiguration& operator=(PermissionConfiguration&& other) = default;

  bool has_type() const { return permission_.has_value(); }
  const PermissionType& type() const { return permission_.value(); }

  const std::string& service() const { return service_; }

 private:
  base::Optional<PermissionType> permission_;
  std::string service_;

  DISALLOW_COPY_AND_ASSIGN(PermissionConfiguration);
};

void CapabilityBroker::BindInterfaceForFrame(
    RenderFrameHost* frame,
    service_manager::BinderRegistry* registry,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  // If the interface requires no permissions, skip permissions checks.
  auto it = permissions_.find(interface_name);
  if (it == permissions_.end() || !it->second.has_type()) {
    GetInterface(frame, registry, interface_name, std::move(interface_pipe),
                 it != permissions_.end() ? &it->second : nullptr);
    return;
  }

  // Otherwise, permission manager decides if access should be granted.
  auto* permission_manager =
      frame->GetProcess()->GetBrowserContext()->GetPermissionManager();

  auto done = base::Bind(
      &CapabilityBroker::RequestPermissionDone, base::Unretained(this), frame,
      registry, interface_name, base::Passed(&interface_pipe), &it->second);

  bool processing_user_gesture = true;  // FIXME.

  permission_manager->RequestPermission(
      it->second.type(), frame, frame->GetLastCommittedOrigin().GetURL(),
      processing_user_gesture, done);
}

CapabilityBroker::CapabilityBroker() {
  // Some test interfaces: FIXME: list all the host interfaces here? Read
  // their configuration from some json5 config file?

  permissions_.emplace(  // Example that requires web permissions.
      "device::mojom::Geolocation",
      PermissionConfiguration(PermissionType::GEOLOCATION, ""));

  permissions_.emplace(  // Example that connects via service manager.
      "shape_detection::mojom::TextDetection",
      PermissionConfiguration(base::nullopt, "shape_detection"));
}

CapabilityBroker::~CapabilityBroker() = default;

void CapabilityBroker::RequestPermissionDone(
    RenderFrameHost* frame,
    service_manager::BinderRegistry* registry,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe,
    PermissionConfiguration* configuration,
    blink::mojom::PermissionStatus result) {
  // LOG(INFO) << __FUNCTION__ << " " << interface_name << " " << result;

  if (result != blink::mojom::PermissionStatus::GRANTED)
    return;  // Permission denied: block access to |interface_name|.

  GetInterface(frame, registry, interface_name, std::move(interface_pipe),
               configuration);
}

void CapabilityBroker::GetInterface(
    RenderFrameHost* frame,
    service_manager::BinderRegistry* registry,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe,
    PermissionConfiguration* configuration) {
  DCHECK(interface_pipe->is_valid());

  if (configuration && !configuration->service().empty()) {
    GetConnectorForFrameHost(frame)->BindInterface(
        service_manager::Identity(configuration->service()), interface_name,
        std::move(interface_pipe));
    return;
  }

  if (registry && registry->TryBindInterface(interface_name, &interface_pipe))
    return;

  if (auto* delegate = GetDelegateForFrameHost(frame))
    delegate->OnInterfaceRequest(frame, interface_name, &interface_pipe);

  if (interface_pipe->is_valid()) {
    GetContentClient()->browser()->BindInterfaceRequestFromFrame(
        frame, interface_name, std::move(interface_pipe));
  }
}

}  // namespace content
