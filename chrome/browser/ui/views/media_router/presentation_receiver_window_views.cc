// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/presentation_receiver_window_views.h"

#include <algorithm>
#include <vector>

#include "base/logging.h"
#include "base/stl_util.h"
#include "chrome/browser/media/router/receiver_presentation_service_delegate_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/media_router/presentation_receiver_window_frame.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "third_party/WebKit/public/web/WebPresentationReceiverFlags.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

using content::WebContents;

namespace media_router {
namespace {

WebContents::CreateParams CreateWebContentsParams(Profile* profile) {
  WebContents::CreateParams params(profile);
  params.starting_sandbox_flags = blink::kPresentationReceiverSandboxFlags;
  return params;
}

}  // namespace

PresentationReceiverWindowViews::~PresentationReceiverWindowViews() {
  DCHECK(!presentation_web_contents_);
  DCHECK(!window_);
}

void PresentationReceiverWindowViews::Start(const std::string& presentation_id,
                                            const GURL& start_url) {
  DCHECK(window_);
  DCHECK(presentation_web_contents_);

  ReceiverPresentationServiceDelegateImpl::CreateForWebContents(
      presentation_web_contents_.get(), presentation_id);

  auto* render_view_host = presentation_web_contents_->GetRenderViewHost();
  DCHECK(render_view_host);
  auto web_prefs = render_view_host->GetWebkitPreferences();
  web_prefs.presentation_receiver = true;
  render_view_host->UpdateWebkitPreferences(web_prefs);

  content::NavigationController::LoadURLParams load_params(start_url);
  load_params.should_replace_current_entry = true;
  load_params.should_clear_history_list = true;
  presentation_web_contents_->GetController().LoadURLWithParams(load_params);

  window_->ShowInactive();
  window_->SetFullscreen(true);
}

void PresentationReceiverWindowViews::Terminate(base::OnceClosure callback) {
  // callback -> !termination_callback_
  DCHECK(callback.is_null() || termination_callback_.is_null());
  if (!callback.is_null()) {
    termination_callback_ = std::move(callback);
  }
  if (presentation_web_contents_) {
    presentation_web_contents_->ClosePage();
  } else if (window_) {
    window_->Close();
  } else {
    if (!termination_callback_.is_null()) {
      std::move(termination_callback_).Run();
      termination_callback_.Reset();
    }
  }
}

content::WebContents*
PresentationReceiverWindowViews::presentation_web_contents() const {
  return presentation_web_contents_.get();
}

views::Widget* PresentationReceiverWindowViews::window() const {
  return window_;
}

PresentationReceiverWindowViews::PresentationReceiverWindowViews(
    Profile* profile,
    const gfx::Rect& bounds)
    : profile_(IndependentOTRProfileManager::GetInstance()
                   ->CreateFromOriginalProfile(
                       profile,
                       base::BindOnce(&PresentationReceiverWindowViews::
                                          OriginalProfileDestroyed,
                                      base::Unretained(this)))),
      window_(new PresentationReceiverWindowFrame(profile_->profile())),
      presentation_web_contents_(
          WebContents::Create(CreateWebContentsParams(profile_->profile()))) {
  DCHECK(profile_->profile());
  DCHECK(profile_->profile()->IsOffTheRecord());
  content::WebContentsObserver::Observe(presentation_web_contents_.get());
  presentation_web_contents_->SetDelegate(this);
  InitWindowWidgetAndViews(bounds);
}

void PresentationReceiverWindowViews::InitWindowWidgetAndViews(
    const gfx::Rect& bounds) {
  DCHECK(presentation_web_contents_);

  auto receiver_view = std::make_unique<PresentationReceiverWindowView>(this);
  receiver_view_ = receiver_view.get();
  window_->InitReceiverFrame(std::move(receiver_view), bounds);
  receiver_view_->Init();
}

void PresentationReceiverWindowViews::OriginalProfileDestroyed(
    Profile* profile) {
  DCHECK(profile == profile_->profile());
  presentation_web_contents_.reset();
  profile_.reset();
  Terminate(base::OnceClosure());
}

void PresentationReceiverWindowViews::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (!navigation_policy_.AllowNavigation(handle))
    Terminate(base::OnceClosure());
}

void PresentationReceiverWindowViews::TitleWasSet(
    content::NavigationEntry* entry) {
  receiver_view_->set_title(entry->GetTitle());
  window_->UpdateWindowTitle();
}

void PresentationReceiverWindowViews::NavigationStateChanged(
    WebContents* source,
    content::InvalidateTypes changed_flags) {
  receiver_view_->UpdateLocationBar();
}

void PresentationReceiverWindowViews::CloseContents(
    content::WebContents* source) {
  presentation_web_contents_.reset();
  Terminate(base::OnceClosure());
}

bool PresentationReceiverWindowViews::ShouldSuppressDialogs(
    content::WebContents* source) {
  DCHECK_EQ(presentation_web_contents(), source);
  // Suppress all because there is no possible direct user interaction with
  // dialogs.
  // TODO(crbug.com/734191): This does not suppress window.print().
  return true;
}

bool PresentationReceiverWindowViews::ShouldFocusLocationBarByDefault(
    content::WebContents* source) {
  DCHECK_EQ(presentation_web_contents(), source);
  // Indicate the location bar should be focused instead of the page, even
  // though there is no location bar.  This will prevent the page from
  // automatically receiving input focus, which should never occur since there
  // is not supposed to be any direct user interaction.
  return true;
}

bool PresentationReceiverWindowViews::ShouldFocusPageAfterCrash() {
  // Never focus the page after a crash.
  return false;
}

void PresentationReceiverWindowViews::CanDownload(
    const GURL& url,
    const std::string& request_method,
    const base::Callback<void(bool)>& callback) {
  // Local presentation pages are not allowed to download files.
  callback.Run(false);
}

bool PresentationReceiverWindowViews::ShouldCreateWebContents(
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

void PresentationReceiverWindowViews::WindowClosing() {
  window_ = nullptr;
  Terminate(base::OnceClosure());
}

WebContents* PresentationReceiverWindowViews::GetWebContents() const {
  return presentation_web_contents();
}

}  // namespace media_router
