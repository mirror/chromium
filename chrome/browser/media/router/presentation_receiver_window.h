// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/media/router/independent_otr_profile_manager.h"
#include "chrome/browser/media/router/presentation_navigation_policy.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"

class BrowserWindow;
class GURL;
class Profile;

namespace gfx {
class Rect;
}  // namespace gfx

namespace media_router {

// This class creates a popup-style window and renders a Presentation API
// receiver page in it.  It also establishes the Presentation API communication
// channel between the receiver and any controlling pages.  This is to support
// local screen casting via the Presentation API.
class PresentationReceiverWindow : public content::WebContentsDelegate,
                                   public content::WebContentsObserver {
 public:
  // Factory method for creating a window from the original user profile.  This
  // will create a new OTR profile (separate from the "user" OTR profile) for
  // the window.
  static std::unique_ptr<PresentationReceiverWindow> CreateFromOriginalProfile(
      Profile* profile,
      const gfx::Rect& bounds);

  ~PresentationReceiverWindow() override;

  void Start(const GURL& start_url, const std::string& presentation_id);
  void Terminate();

  content::WebContents* presentation_web_contents() const {
    return browser_->tab_strip_model()->GetWebContentsAt(0);
  }

  const Browser* browser() const { return browser_; }

  BrowserWindow* window() const { return browser_->window(); }

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
  PresentationReceiverWindow(
      IndependentOTRProfileManager::OTRProfileRegistration profile,
      const gfx::Rect& bounds);

  // The Browser object for the window we created for the receiver.
  Browser* browser_;

  // Manages the lifetime of the OTR profile used for the presentation.
  IndependentOTRProfileManager::OTRProfileRegistration profile_;

  PresentationNavigationPolicy navigation_policy_;

  DISALLOW_COPY_AND_ASSIGN(PresentationReceiverWindow);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_
