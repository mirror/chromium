// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation_receiver_window.h"

#include <algorithm>
#include <vector>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/sequence_checker.h"
#include "base/stl_util.h"
#include "chrome/browser/media/router/receiver_presentation_service_delegate_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

namespace media_router {
namespace {

std::vector<Profile*>& g_receiver_profiles() {
  static std::vector<Profile*> profiles;
  return profiles;
}

Browser::CreateParams MakeBrowserCreateParams(Profile* profile,
                                              const gfx::Rect& bounds) {
  Browser::CreateParams params(profile, true);
  params.type = Browser::TYPE_POPUP;
  params.initial_bounds = bounds;
  return params;
}

}  // namespace

// static
void PresentationReceiverWindow::DeleteProfile(Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // When Terminate() is called, or any other path that will close the window
  // and stop the presentation, this PresentationReceiverWindow will be
  // destroyed.  Since it is a Browser subclass, there are two important things
  // to consider about the lifetime of the profile we use: 1. ~Browser still
  // needs the profile to be valid and 2. ~Browser expects it can delete an OTR
  // profile (except in special cases) after it's done with it.
  //
  // Because of 1, we can't hold profile_ as a std::unique_ptr.  Because of 2,
  // we either need ~Browser or ProfileDestroyer (what Browser uses to safely
  // destroy a Profile) customization.  We _do_ want it to be destroyed so we
  // have added logic to ProfileDestroyer to let us destroy the Profile, since
  // it isn't the normal "user" OTR profile (which is instead owned by
  // ProfileImpl).
  auto& receiver_profiles = g_receiver_profiles();
  auto profile_it =
      std::remove(receiver_profiles.begin(), receiver_profiles.end(), profile);
  if (profile_it != receiver_profiles.end()) {
    receiver_profiles.erase(profile_it, receiver_profiles.end());
    delete profile;
  }
}

// static
bool PresentationReceiverWindow::IsPresentationReceiverWindowProfile(
    const Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return base::ContainsValue(g_receiver_profiles(), profile);
}

// static
PresentationReceiverWindow*
PresentationReceiverWindow::CreateFromOriginalProfile(Profile* profile,
                                                      const GURL& start_url,
                                                      const gfx::Rect& bounds) {
  DCHECK(profile);
  DCHECK(!profile->IsOffTheRecord());
  return new PresentationReceiverWindow(
      base::WrapUnique(profile->CreateOffTheRecordProfile()), start_url,
      bounds);
}

PresentationReceiverWindow::~PresentationReceiverWindow() = default;

void PresentationReceiverWindow::Start(const std::string& presentation_id) {
  presentation_web_contents_ = content::WebContents::Create(
      content::WebContents::CreateParams(profile_));
  content::WebContentsObserver::Observe(presentation_web_contents_);
  ReceiverPresentationServiceDelegateImpl::CreateForWebContents(
      presentation_web_contents_, presentation_id);

  auto* render_view_host = presentation_web_contents_->GetRenderViewHost();
  DCHECK(render_view_host);
  auto web_prefs = render_view_host->GetWebkitPreferences();
  web_prefs.presentation_receiver = true;
  render_view_host->UpdateWebkitPreferences(web_prefs);

  content::NavigationController::LoadURLParams load_params(start_url_);
  load_params.should_replace_current_entry = true;
  load_params.should_clear_history_list = true;
  presentation_web_contents_->GetController().LoadURLWithParams(load_params);

  chrome::NavigateParams nav_params(this, presentation_web_contents_);
  nav_params.window_action = chrome::NavigateParams::SHOW_WINDOW_INACTIVE;
  chrome::Navigate(&nav_params);
  exclusive_access_manager()
      ->fullscreen_controller()
      ->ToggleBrowserFullscreenMode();
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
  // Suppress all because there is no possible direct user interaction with
  // dialogs.
  // TODO(crbug.com/734191): This does not suppress window.print().
  return true;
}

bool PresentationReceiverWindow::ShouldFocusLocationBarByDefault(
    content::WebContents* source) {
  DCHECK_EQ(presentation_web_contents_, source);
  // Indicate the location bar should be focused instead of the page, even
  // though there is no location bar.  This will prevent the page from
  // automatically receiving input focus, which should never occur since there
  // is not supposed to be any direct user interaction.
  return true;
}

bool PresentationReceiverWindow::ShouldFocusPageAfterCrash() {
  // Never focus the page after a crash.
  return false;
}

void PresentationReceiverWindow::CanDownload(
    const GURL& url,
    const std::string& request_method,
    const base::Callback<void(bool)>& callback) {
  // Local presentation pages are not allowed to download files.
  callback.Run(false);
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
  // Disallow creating separate WebContentses.  The WebContents implementation
  // uses this to spawn new windows/tabs, which is also not allowed for
  // local presentations.
  return false;
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
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  g_receiver_profiles().push_back(profile_);
}

}  // namespace media_router
