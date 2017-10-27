// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_

#include <memory>

#include "chrome/browser/media/router/presentation_navigation_policy.h"
#include "chrome/browser/media/router/presentation_tab_delegate.h"
#include "chrome/browser/ui/browser.h"
#include "ui/gfx/geometry/rect.h"

namespace media_router {

class PresentationReceiverWindow : public Browser,
                                   public content::WebContentsObserver {
 public:
  PresentationReceiverWindow(content::BrowserContext* context,
                             const GURL& start_url,
                             const gfx::Rect& bounds);
  ~PresentationReceiverWindow() override;

  void Start(const std::string& presentation_id);

  content::WebContents* presentation_web_contents() {
    return presentation_web_contents_;
  }

  // content::WebContentsObserver overrides.
  void DidStartNavigation(content::NavigationHandle* handle) final;

  // content::WebContentsDelegate overrides.
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

 private:
  // WebContents of the receiver page.  Owned by Browser after chrome::Navigate.
  content::WebContents* presentation_web_contents_ = nullptr;

  PresentationTabDelegate web_contents_delegate_;
  const GURL start_url_;
  PresentationNavigationPolicy navigation_policy_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_
