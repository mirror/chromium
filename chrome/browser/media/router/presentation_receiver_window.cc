// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation_receiver_window.h"

#include <algorithm>
#include <vector>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "chrome/browser/media/router/receiver_presentation_service_delegate_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

namespace media_router {
namespace {

base::LazyInstance<std::vector<Profile*>>::Leaky g_receiver_profiles =
    LAZY_INSTANCE_INITIALIZER;

Browser::CreateParams MakeBrowserCreateParams(Profile* profile,
                                              const gfx::Rect& bounds) {
  auto params = Browser::CreateParams(profile, true);
  params.type = Browser::TYPE_POPUP;
  params.initial_bounds = bounds;
  return params;
}

}  // namespace

// static
void PresentationReceiverWindow::DeleteProfile(Profile* profile) {
  auto& receiver_profiles = g_receiver_profiles.Get();
  receiver_profiles.erase(
      std::remove(receiver_profiles.begin(), receiver_profiles.end(), profile),
      receiver_profiles.end());
  delete profile;
}

// static
bool PresentationReceiverWindow::IsPresentationReceiverWindowProfile(
    const Profile* profile) {
  const auto& receiver_profiles = g_receiver_profiles.Get();
  return std::find(receiver_profiles.begin(), receiver_profiles.end(),
                   profile) != receiver_profiles.end();
}

PresentationReceiverWindow::PresentationReceiverWindow(
    std::unique_ptr<Profile> window_profile,
    const GURL& start_url,
    const gfx::Rect& bounds)
    : Browser(MakeBrowserCreateParams(window_profile.get(), bounds)),
      profile_(window_profile.release()),
      start_url_(start_url) {
  DCHECK(profile_);
  DCHECK(profile_->IsOffTheRecord());
  // This means that the profile we were given for the window is a separate
  // off-the-record profile from what the user would normally use.  In other
  // words, it was created with Profile::CreateOffTheRecordProfile() and we own
  // it.
  DCHECK(
      !profile_->GetOriginalProfile()->HasOffTheRecordProfile() ||
      (profile_->GetOriginalProfile()->GetOffTheRecordProfile() != profile_));
  g_receiver_profiles.Get().push_back(profile_);
}

PresentationReceiverWindow::~PresentationReceiverWindow() = default;

void PresentationReceiverWindow::Start(const std::string& presentation_id) {
  presentation_web_contents_ = content::WebContents::Create(
      content::WebContents::CreateParams(profile_));
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

void PresentationReceiverWindow::Terminate() {
  if (presentation_web_contents_) {
    presentation_web_contents_->ClosePage();
  }
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
