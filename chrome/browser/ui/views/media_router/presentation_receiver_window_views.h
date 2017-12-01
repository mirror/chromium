// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_VIEWS_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/media/router/independent_otr_profile_manager.h"
#include "chrome/browser/media/router/presentation_navigation_policy.h"
#include "chrome/browser/media/router/presentation_receiver_window.h"
#include "chrome/browser/ui/views/media_router/presentation_receiver_window_view.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"

class GURL;
class Profile;

namespace content {
class WebContents;
}

namespace views {
class Widget;
}

namespace gfx {
class Rect;
}

class PresentationReceiverWindowFrame;

namespace media_router {

// This class implements PresentationReceiverWindow using Views.  It shows the
// receiver page in a Widget window with a URL bar.
class PresentationReceiverWindowViews final
    : public PresentationReceiverWindow,
      public content::WebContentsDelegate,
      public content::WebContentsObserver,
      public PresentationReceiverWindowView::Controller {
 public:
  ~PresentationReceiverWindowViews() final;

 private:
  friend class PresentationReceiverWindowFactory;

  PresentationReceiverWindowViews(Profile* profile, const gfx::Rect& bounds);

  void InitWindowWidgetAndViews(const gfx::Rect& bounds);

  void OriginalProfileDestroyed(Profile* profile);

  // PresentationReceiverWindow overrides.
  void Start(const std::string& presentation_id, const GURL& start_url) final;
  void Terminate(base::OnceClosure callback) final;
  content::WebContents* presentation_web_contents() const final;
  views::Widget* window() const final;

  // content::WebContentsObserver overrides.
  void DidStartNavigation(content::NavigationHandle* handle) final;
  void TitleWasSet(content::NavigationEntry* entry) final;

  // content::WebContentsDelegate overrides.
  void NavigationStateChanged(content::WebContents* source,
                              content::InvalidateTypes changed_flags) final;
  void CloseContents(content::WebContents* source) final;
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

  // PresentationReceiverWindowView::Controller overrides.
  void WindowClosing() final;
  content::WebContents* GetWebContents() const final;

  // The profile used for the presentation.
  std::unique_ptr<IndependentOTRProfileManager::OTRProfileRegistration>
      profile_;

  // Owned by its native widget.
  PresentationReceiverWindowFrame* window_;

  // WebContents for rendering the receiver page.
  std::unique_ptr<content::WebContents> presentation_web_contents_;

  // Owned by the frame.
  PresentationReceiverWindowView* receiver_view_ = nullptr;

  base::OnceClosure termination_callback_;

  PresentationNavigationPolicy navigation_policy_;

  DISALLOW_COPY_AND_ASSIGN(PresentationReceiverWindowViews);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_VIEWS_H_
