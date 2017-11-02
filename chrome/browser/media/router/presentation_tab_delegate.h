// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_TAB_DELEGATE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_TAB_DELEGATE_H_

#include "content/public/browser/web_contents_delegate.h"

namespace media_router {

class PresentationTabDelegate : public content::WebContentsDelegate {
 public:
  // content::WebContentsDelegate overrides to provide the desired behaviors.
  bool ShouldSuppressDialogs(content::WebContents* source) final;
  bool ShouldFocusLocationBarByDefault(content::WebContents* source) final;
  bool ShouldFocusPageAfterCrash() final;
  void CanDownload(const GURL& url,
                   const std::string& request_method,
                   const base::Callback<void(bool)>& callback) final;
  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      int32_t route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) final;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_TAB_DELEGATE_H_
