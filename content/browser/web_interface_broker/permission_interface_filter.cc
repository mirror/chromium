// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/web_interface_filter.h"

#include <memory>
#include <utility>

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

class PermissionInterfaceFilter : public WebInterfaceFilter {
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

}  // namespace

std::unique_ptr<WebInterfaceFilter> CreatePermissionInterfaceFilter(
    PermissionType type) {
  return std::make_unique<PermissionInterfaceFilter>(type);
}

}  // namespace content
