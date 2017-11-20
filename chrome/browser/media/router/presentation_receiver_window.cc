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
#include "chrome/browser/ui/browser_list.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "third_party/WebKit/public/web/WebPresentationReceiverFlags.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

namespace media_router {
namespace {

Browser::CreateParams MakeBrowserCreateParams(Profile* profile,
                                              const gfx::Rect& bounds) {
  Browser::CreateParams params(profile, true);
  params.type = Browser::TYPE_POPUP;
  params.initial_bounds = bounds;
  return params;
}

}  // namespace

// static
std::unique_ptr<PresentationReceiverWindow>
PresentationReceiverWindow::CreateFromOriginalProfile(Profile* profile,
                                                      const gfx::Rect& bounds) {
  DCHECK(profile);
  DCHECK(!profile->IsOffTheRecord());
  return base::WrapUnique(new PresentationReceiverWindow(
      IndependentOTRProfileManager::GetInstance()->CreateFromOriginalProfile(
          profile),
      bounds));
}

PresentationReceiverWindow::~PresentationReceiverWindow() {
  Terminate();
}

void PresentationReceiverWindow::Start(const GURL& start_url,
                                       const std::string& presentation_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::WebContents::CreateParams params(profile_.profile());
  params.starting_sandbox_flags = blink::kPresentationReceiverSandboxFlags;
  auto* presentation_web_contents = content::WebContents::Create(params);
  content::WebContentsObserver::Observe(presentation_web_contents);
  ReceiverPresentationServiceDelegateImpl::CreateForWebContents(
      presentation_web_contents, presentation_id);

  auto* render_view_host = presentation_web_contents->GetRenderViewHost();
  DCHECK(render_view_host);
  auto web_prefs = render_view_host->GetWebkitPreferences();
  web_prefs.presentation_receiver = true;
  render_view_host->UpdateWebkitPreferences(web_prefs);

  content::NavigationController::LoadURLParams load_params(start_url);
  load_params.should_replace_current_entry = true;
  load_params.should_clear_history_list = true;
  presentation_web_contents->GetController().LoadURLWithParams(load_params);

  chrome::NavigateParams nav_params(browser_, presentation_web_contents);
  nav_params.window_action = chrome::NavigateParams::SHOW_WINDOW_INACTIVE;
  chrome::Navigate(&nav_params);
  browser_->exclusive_access_manager()
      ->fullscreen_controller()
      ->ToggleBrowserFullscreenMode();
}

void PresentationReceiverWindow::Terminate() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (base::ContainsValue(*BrowserList::GetInstance(), browser_) &&
      browser_->tab_strip_model()->count() == 1) {
    presentation_web_contents()->ClosePage();
  }
}

void PresentationReceiverWindow::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (!navigation_policy_.AllowNavigation(handle)) {
    Terminate();
  }
}

bool PresentationReceiverWindow::ShouldSuppressDialogs(
    content::WebContents* source) {
  DCHECK_EQ(presentation_web_contents(), source);
  // Suppress all because there is no possible direct user interaction with
  // dialogs.
  // TODO(crbug.com/734191): This does not suppress window.print().
  return true;
}

bool PresentationReceiverWindow::ShouldFocusLocationBarByDefault(
    content::WebContents* source) {
  DCHECK_EQ(presentation_web_contents(), source);
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
  DCHECK_EQ(presentation_web_contents(), web_contents);
  // Disallow creating separate WebContentses.  The WebContents implementation
  // uses this to spawn new windows/tabs, which is also not allowed for
  // local presentations.
  return false;
}

PresentationReceiverWindow::PresentationReceiverWindow(
    IndependentOTRProfileManager::OTRProfileRegistration profile,
    const gfx::Rect& bounds)
    : browser_(new Browser(MakeBrowserCreateParams(profile.profile(), bounds))),
      profile_(std::move(profile)) {
  DCHECK(profile_.profile());
  DCHECK(profile_.profile()->IsOffTheRecord());
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

}  // namespace media_router
