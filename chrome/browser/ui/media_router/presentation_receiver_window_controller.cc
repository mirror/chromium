// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/media_router/presentation_receiver_window_controller.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/media/router/receiver_presentation_service_delegate_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/media_router/presentation_receiver_window.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "third_party/WebKit/public/web/WebPresentationReceiverFlags.h"
#include "ui/views/widget/widget.h"

using content::WebContents;

namespace {

WebContents::CreateParams CreateWebContentsParams(Profile* profile) {
  WebContents::CreateParams params(profile);
  params.starting_sandbox_flags = blink::kPresentationReceiverSandboxFlags;
  return params;
}

}  // namespace

// static
std::unique_ptr<PresentationReceiverWindowController>
PresentationReceiverWindowController::CreateFromOriginalProfile(
    Profile* profile,
    const gfx::Rect& bounds) {
  DCHECK(profile);
  DCHECK(!profile->IsOffTheRecord());
  return base::WrapUnique(
      new PresentationReceiverWindowController(profile, bounds));
}

PresentationReceiverWindowController::~PresentationReceiverWindowController() {
  DCHECK(!web_contents_);
  DCHECK(!window_);
}

void PresentationReceiverWindowController::Start(
    const std::string& presentation_id,
    const GURL& start_url) {
  DCHECK(window_);
  DCHECK(web_contents_);

  media_router::ReceiverPresentationServiceDelegateImpl::CreateForWebContents(
      web_contents_.get(), presentation_id);

  auto* render_view_host = web_contents_->GetRenderViewHost();
  DCHECK(render_view_host);
  auto web_prefs = render_view_host->GetWebkitPreferences();
  web_prefs.presentation_receiver = true;
  render_view_host->UpdateWebkitPreferences(web_prefs);

  content::NavigationController::LoadURLParams load_params(start_url);
  load_params.should_replace_current_entry = true;
  load_params.should_clear_history_list = true;
  web_contents_->GetController().LoadURLWithParams(load_params);

  window_->ShowInactiveFullscreen();
}

void PresentationReceiverWindowController::Terminate(
    base::OnceClosure callback) {
  // callback -> !termination_callback_
  DCHECK(callback.is_null() || termination_callback_.is_null());
  if (!callback.is_null()) {
    termination_callback_ = std::move(callback);
  }
  if (web_contents_) {
    web_contents_->ClosePage();
  } else if (window_) {
    window_->Close();
  } else {
    if (!termination_callback_.is_null()) {
      std::move(termination_callback_).Run();
      termination_callback_.Reset();
    }
  }
}

void PresentationReceiverWindowController::CloseWindowForTest() {
  window_->Close();
}

bool PresentationReceiverWindowController::IsWindowActiveForTest() const {
  return window_->IsWindowActive();
}

bool PresentationReceiverWindowController::IsWindowFullscreenForTest() const {
  return window_->IsWindowFullscreen();
}

gfx::Rect PresentationReceiverWindowController::GetWindowBoundsForTest() const {
  return window_->GetWindowBounds();
}

WebContents* PresentationReceiverWindowController::web_contents() const {
  return web_contents_.get();
}

PresentationReceiverWindowController::PresentationReceiverWindowController(
    Profile* profile,
    const gfx::Rect& bounds)
    : profile_(IndependentOTRProfileManager::GetInstance()
                   ->CreateFromOriginalProfile(
                       profile,
                       base::BindOnce(&PresentationReceiverWindowController::
                                          OriginalProfileDestroyed,
                                      base::Unretained(this)))),
      web_contents_(
          WebContents::Create(CreateWebContentsParams(profile_->profile()))),
      window_(PresentationReceiverWindow::Create(this, bounds)) {
  DCHECK(profile_->profile());
  DCHECK(profile_->profile()->IsOffTheRecord());
  content::WebContentsObserver::Observe(web_contents_.get());
  web_contents_->SetDelegate(this);
}

void PresentationReceiverWindowController::WindowClosing() {
  window_ = nullptr;
  Terminate(base::OnceClosure());
}

void PresentationReceiverWindowController::OriginalProfileDestroyed(
    Profile* profile) {
  DCHECK(profile == profile_->profile());
  web_contents_.reset();
  profile_.reset();
  Terminate(base::OnceClosure());
}

void PresentationReceiverWindowController::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (!navigation_policy_.AllowNavigation(handle))
    Terminate(base::OnceClosure());
}

void PresentationReceiverWindowController::TitleWasSet(
    content::NavigationEntry* entry) {
  window_->UpdateWindowTitle();
}

void PresentationReceiverWindowController::NavigationStateChanged(
    WebContents* source,
    content::InvalidateTypes changed_flags) {
  window_->UpdateLocationBar();
}

void PresentationReceiverWindowController::CloseContents(
    content::WebContents* source) {
  web_contents_.reset();
  Terminate(base::OnceClosure());
}

bool PresentationReceiverWindowController::ShouldSuppressDialogs(
    content::WebContents* source) {
  DCHECK_EQ(web_contents_.get(), source);
  // Suppress all because there is no possible direct user interaction with
  // dialogs.
  // TODO(crbug.com/734191): This does not suppress window.print().
  return true;
}

bool PresentationReceiverWindowController::ShouldFocusLocationBarByDefault(
    content::WebContents* source) {
  DCHECK_EQ(web_contents_.get(), source);
  // Indicate the location bar should be focused instead of the page, even
  // though there is no location bar.  This will prevent the page from
  // automatically receiving input focus, which should never occur since there
  // is not supposed to be any direct user interaction.
  return true;
}

bool PresentationReceiverWindowController::ShouldFocusPageAfterCrash() {
  // Never focus the page after a crash.
  return false;
}

void PresentationReceiverWindowController::CanDownload(
    const GURL& url,
    const std::string& request_method,
    const base::Callback<void(bool)>& callback) {
  // Local presentation pages are not allowed to download files.
  callback.Run(false);
}

bool PresentationReceiverWindowController::ShouldCreateWebContents(
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
  DCHECK_EQ(web_contents_.get(), web_contents);
  // Disallow creating separate WebContentses.  The WebContents implementation
  // uses this to spawn new windows/tabs, which is also not allowed for
  // local presentations.
  return false;
}
