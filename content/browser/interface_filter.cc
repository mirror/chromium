// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/interfaces/interface_filter.h"

#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace url {
class Origin;
}

namespace content {
namespace {

const url::Origin& GetEmbeddingOrigin(RenderFrameHost* frame,
                                      const url::Origin& origin) {
  if (!frame)
    return origin;

  return WebContents::FromRenderFrameHost(frame)
      ->GetMainFrame()
      ->GetLastCommittedOrigin();
}

class PermissionInterfaceFilter : public InterfaceFilter {
 public:
  PermissionInterfaceFilter(PermissionType type);

  bool IsAllowed(WebContextType context_type,
                 RenderProcessHost* process_host,
                 RenderFrameHost* frame_host,
                 const url::Origin& origin) override;

 private:
  PermissionType type_;
};

PermissionInterfaceFilter::PermissionInterfaceFilter(PermissionType type)
    : type_(type) {}

bool PermissionInterfaceFilter::IsAllowed(WebContextType context_type,
                                          RenderProcessHost* process_host,
                                          RenderFrameHost* frame_host,
                                          const url::Origin& origin) {
  return process_host->GetBrowserContext()
             ->GetPermissionManager()
             ->GetPermissionStatus(
                 type_, origin.GetURL(),
                 GetEmbeddingOrigin(frame_host, origin).GetURL()) ==
         blink::mojom::PermissionStatus::GRANTED;
}

class ContextTypeInterfaceFilter : public InterfaceFilter {
 public:
  ContextTypeInterfaceFilter(
      std::initializer_list<WebContextType> allowed_contexts)
      : context_types_(allowed_contexts) {}

  bool IsAllowed(WebContextType context_type,
                 RenderProcessHost* process_host,
                 RenderFrameHost* frame_host,
                 const url::Origin& origin) override {
    return std::find(context_types_.begin(), context_types_.end(),
                     context_type) != context_types_.end();
  }

 private:
  std::vector<WebContextType> context_types_;
};

}  // namespace

std::unique_ptr<InterfaceFilter> CreatePermissionInterfaceFilter(
    PermissionType type) {
  return std::make_unique<PermissionInterfaceFilter>(type);
}

std::unique_ptr<InterfaceFilter> CreateContextTypeInterfaceFilter(
    std::initializer_list<WebContextType> allowed_contexts) {
  return std::make_unique<ContextTypeInterfaceFilter>(allowed_contexts);
}

InterfaceConfiguration::InterfaceConfiguration(InterfaceConfiguration&&) =
    default;
InterfaceConfiguration& InterfaceConfiguration::operator=(
    InterfaceConfiguration&&) = default;
InterfaceConfiguration::~InterfaceConfiguration() = default;

bool InterfaceConfiguration::IsAllowed(WebContextType type,
                                       RenderProcessHost* process_host,
                                       RenderFrameHost* frame_host,
                                       const url::Origin& origin) {
  for (auto& filter : filters_) {
    if (!filter->IsAllowed(type, process_host, frame_host, origin)) {
      return false;
    }
  }
  return true;
}

}  // namespace content
