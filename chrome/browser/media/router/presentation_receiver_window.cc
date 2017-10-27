// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation_receiver_window.h"

#include "base/logging.h"
#include "chrome/browser/media/router/receiver_presentation_service_delegate_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/web_preferences.h"

namespace media_router {
namespace {

Browser::CreateParams MakeBrowserCreateParams(content::BrowserContext* context,
                                              const gfx::Rect& bounds) {
  auto params =
      Browser::CreateParams(Profile::FromBrowserContext(context), true);
  params.type = Browser::TYPE_POPUP;
  params.initial_bounds = bounds;
  return params;
}

}  // namespace

PresentationReceiverWindow::PresentationReceiverWindow(
    content::BrowserContext* context,
    const GURL& start_url,
    const gfx::Rect& bounds)
    : Browser(MakeBrowserCreateParams(context, bounds)),
      start_url_(start_url) {}

PresentationReceiverWindow::~PresentationReceiverWindow() = default;

void PresentationReceiverWindow::Start(const std::string& presentation_id) {
  // TODO: Profile ownership, type, and difference between Browser/WebContents
  // is a little unclear.  This doesn't seem right but so far I get CHECK
  // failures on exit if I do this differently.
  presentation_web_contents_ = content::WebContents::Create(
      content::WebContents::CreateParams(profile()->GetOffTheRecordProfile()));
  presentation_web_contents_->SetDelegate(this);
  content::WebContentsObserver::Observe(presentation_web_contents_);
  ReceiverPresentationServiceDelegateImpl::CreateForWebContents(
      presentation_web_contents_, presentation_id);
  if (auto* render_view_host =
          presentation_web_contents_->GetRenderViewHost()) {
    auto web_prefs = render_view_host->GetWebkitPreferences();
    web_prefs.presentation_receiver = true;
    render_view_host->UpdateWebkitPreferences(web_prefs);
  }

  // TODO: Should this feature be added to the LocationBar API?  Illegal
  // includes as-is and hacky anyway.
  // static_cast<BrowserView*>(window())
  //     ->GetLocationBarView()
  //     ->omnibox_view()
  //     ->SetReadOnly(true);
  chrome::NavigateParams nav_params(this, presentation_web_contents_);
  // TODO: Test for inactive window?
  nav_params.window_action = chrome::NavigateParams::SHOW_WINDOW_INACTIVE;
  chrome::Navigate(&nav_params);
  exclusive_access_manager()
      ->fullscreen_controller()
      ->ToggleBrowserFullscreenMode();

  content::NavigationController::LoadURLParams load_params(start_url_);
  load_params.should_replace_current_entry = true;
  load_params.should_clear_history_list = true;
  presentation_web_contents_->GetController().LoadURLWithParams(load_params);
}

void PresentationReceiverWindow::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (!navigation_policy_.AllowNavigation(handle)) {
    presentation_web_contents_->ClosePage();
  }
}

bool PresentationReceiverWindow::ShouldSuppressDialogs(
    content::WebContents* source) {
  DCHECK_EQ(presentation_web_contents_, source);
  return web_contents_delegate_.ShouldSuppressDialogs(source);
}

bool PresentationReceiverWindow::ShouldFocusLocationBarByDefault(
    content::WebContents* source) {
  DCHECK_EQ(presentation_web_contents_, source);
  return web_contents_delegate_.ShouldFocusLocationBarByDefault(source);
}

bool PresentationReceiverWindow::ShouldFocusPageAfterCrash() {
  return web_contents_delegate_.ShouldFocusPageAfterCrash();
}

void PresentationReceiverWindow::CanDownload(
    const GURL& url,
    const std::string& request_method,
    const base::Callback<void(bool)>& callback) {
  web_contents_delegate_.CanDownload(url, request_method, callback);
}

bool PresentationReceiverWindow::ShouldCreateWebContents(
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
    content::SessionStorageNamespace* session_storage_namespace) {
  DCHECK_EQ(presentation_web_contents_, web_contents);
  return web_contents_delegate_.ShouldCreateWebContents(
      web_contents, opener, source_site_instance, route_id, main_frame_route_id,
      main_frame_widget_route_id, window_container_type, opener_url, frame_name,
      target_url, partition_id, session_storage_namespace);
}

}  // namespace media_router
