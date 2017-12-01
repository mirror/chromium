// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/interfaces/interface_filter.h"

#include "base/containers/flat_set.h"
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
  explicit PermissionInterfaceFilter(PermissionType type) : type_(type) {}

  Result FilterInterface(WebContextType context_type,
                         RenderProcessHost* process_host,
                         RenderFrameHost* frame_host,
                         const url::Origin& origin) override {
    return process_host->GetBrowserContext()
                       ->GetPermissionManager()
                       ->GetPermissionStatus(
                           type_, origin.GetURL(),
                           GetEmbeddingOrigin(frame_host, origin).GetURL()) ==
                   blink::mojom::PermissionStatus::GRANTED
               ? Result::kAllow
               : Result::kDeny;
  }

 private:
  PermissionType type_;
};

class ContextTypeInterfaceFilter : public InterfaceFilter {
 public:
  explicit ContextTypeInterfaceFilter(
      std::initializer_list<WebContextType> allowed_contexts)
      : context_types_(allowed_contexts, base::KEEP_FIRST_OF_DUPES) {}

  Result FilterInterface(WebContextType context_type,
                         RenderProcessHost* process_host,
                         RenderFrameHost* frame_host,
                         const url::Origin& origin) override {
    return context_types_.find(context_type) != context_types_.end()
               ? Result::kAllow
               : Result::kBadMessage;
  }

 private:
  base::flat_set<WebContextType> context_types_;
};

InterfaceFilter::Result ResolveResults(InterfaceFilter::Result a,
                                       InterfaceFilter::Result b) {
  if (a == InterfaceFilter::Result::kBadMessage ||
      b == InterfaceFilter::Result::kBadMessage) {
    return InterfaceFilter::Result::kBadMessage;
  }
  if (a == InterfaceFilter::Result::kDeny ||
      b == InterfaceFilter::Result::kDeny) {
    return InterfaceFilter::Result::kDeny;
  }
  return InterfaceFilter::Result::kAllow;
}

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

InterfaceFilter::Result InterfaceConfiguration::FilterInterface(
    WebContextType type,
    RenderProcessHost* process_host,
    RenderFrameHost* frame_host,
    const url::Origin& origin) {
  auto result = InterfaceFilter::Result::kAllow;
  for (auto& filter : filters_) {
    result = ResolveResults(
        result,
        filter->FilterInterface(type, process_host, frame_host, origin));
    if (result == InterfaceFilter::Result::kBadMessage) {
      return InterfaceFilter::Result::kBadMessage;
    }
  }
  return result;
}

}  // namespace content
