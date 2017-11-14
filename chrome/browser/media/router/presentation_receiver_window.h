// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_

#include <memory>
#include <string>

#include "chrome/browser/media/router/presentation_navigation_policy.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"

class GURL;
class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace views {
class WebView;
class Widget;
}  // namespace views

namespace gfx {
class Rect;
}  // namespace gfx

namespace media_router {

class PresentationReceiverWindow : public content::WebContentsDelegate,
                                   public content::WebContentsObserver,
                                   public content::NotificationObserver {
 public:
  // Factory method for creating a window from the original user profile.  This
  // will create a new OTR profile (separate from the "user" OTR profile) for
  // the window.
  static std::unique_ptr<PresentationReceiverWindow> CreateFromOriginalProfile(
      Profile* profile,
      const GURL& start_url,
      const gfx::Rect& bounds);

  ~PresentationReceiverWindow() override;

  void Start(const std::string& presentation_id);
  void Terminate();

  content::WebContents* presentation_web_contents() const {
    return presentation_web_contents_.get();
  }

  views::Widget* window() { return window_; }

  // content::WebContentsObserver overrides.
  void DidStartNavigation(content::NavigationHandle* handle) final;
  void TitleWasSet(content::NavigationEntry* entry) final;

  // content::WebContentsDelegate overrides.
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

  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) final;

 private:
  PresentationReceiverWindow(std::unique_ptr<Profile> profile,
                             const GURL& start_url,
                             const gfx::Rect& bounds);

  void InitWindowWidgetAndViews(const gfx::Rect& bounds);

  // The profile used for the presentation.
  std::unique_ptr<Profile> profile_;

  // Owned by the native widget.
  views::Widget* window_;

  // Owned by the view hierarchy of |window_|.
  views::WebView* web_contents_view_;

  // WebContents for rendering the receiver page.
  std::unique_ptr<content::WebContents> presentation_web_contents_;

  const GURL start_url_;
  PresentationNavigationPolicy navigation_policy_;
  content::NotificationRegistrar registrar_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_
