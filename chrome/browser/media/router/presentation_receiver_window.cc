// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation_receiver_window.h"

#include <algorithm>
#include <vector>

#include "base/logging.h"
#include "base/stl_util.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/media/router/receiver_presentation_service_delegate_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "third_party/WebKit/public/web/WebPresentationReceiverFlags.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "url/gurl.h"

namespace media_router {
namespace {

views::Widget::InitParams CreateWidgetInitParams(
    std::unique_ptr<views::WidgetDelegateView> delegate_view,
    const gfx::Rect& bounds) {
  // TODO: TYPE_WINDOW_FRAMELESS difference?
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);
  params.bounds = bounds;
  params.delegate = delegate_view.release();
  return params;
}

class WindowContents : public views::WidgetDelegateView {
 public:
  WindowContents(PresentationReceiverWindow* parent,
                 std::unique_ptr<views::WebView> web_view)
      : parent_(parent) {
    SetLayoutManager(new views::FillLayout());
    AddChildView(web_view.release());
  }

  // views::WidgetDelegateView overrides.
  void WindowClosing() final {
    if (parent_) {
      parent_->Terminate();
    }
  }
  base::string16 GetWindowTitle() const final { return title_; }

  void set_title(const base::string16& title) {
    title_ = title;
    parent_->window()->UpdateWindowTitle();
  }

  void ParentDestroyed() { parent_ = nullptr; }

 private:
  PresentationReceiverWindow* parent_;
  base::string16 title_;
};

}  // namespace

// static
std::unique_ptr<PresentationReceiverWindow>
PresentationReceiverWindow::CreateFromOriginalProfile(Profile* profile,
                                                      const GURL& start_url,
                                                      const gfx::Rect& bounds) {
  DCHECK(profile);
  DCHECK(!profile->IsOffTheRecord());
  return base::WrapUnique(new PresentationReceiverWindow(
      base::WrapUnique(profile->CreateOffTheRecordProfile()), start_url,
      bounds));
}

PresentationReceiverWindow::~PresentationReceiverWindow() {
  if (window_) {
    static_cast<WindowContents*>(window_->widget_delegate())->ParentDestroyed();
  }
}

void PresentationReceiverWindow::Start(const std::string& presentation_id) {
  DCHECK(!presentation_web_contents_);
  DCHECK(window_);
  DCHECK(web_contents_view_);

  content::WebContents::CreateParams params(profile_.get());
  params.starting_sandbox_flags = blink::kPresentationReceiverSandboxFlags;
  presentation_web_contents_.reset(content::WebContents::Create(params));
  content::WebContentsObserver::Observe(presentation_web_contents_.get());
  presentation_web_contents_->SetDelegate(this);
  ReceiverPresentationServiceDelegateImpl::CreateForWebContents(
      presentation_web_contents_.get(), presentation_id);

  auto* render_view_host = presentation_web_contents_->GetRenderViewHost();
  DCHECK(render_view_host);
  auto web_prefs = render_view_host->GetWebkitPreferences();
  web_prefs.presentation_receiver = true;
  render_view_host->UpdateWebkitPreferences(web_prefs);

  content::NavigationController::LoadURLParams load_params(start_url_);
  load_params.should_replace_current_entry = true;
  load_params.should_clear_history_list = true;
  presentation_web_contents_->GetController().LoadURLWithParams(load_params);

  web_contents_view_->SetWebContents(presentation_web_contents_.get());
  window_->Show();
  window_->SetFullscreen(true);
}

void PresentationReceiverWindow::Terminate() {
  if (window_) {
    window_->Close();
    window_ = nullptr;
  }
  if (presentation_web_contents_) {
    presentation_web_contents_->ClosePage();
  }
}

void PresentationReceiverWindow::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (!navigation_policy_.AllowNavigation(handle)) {
    Terminate();
  }
}

void PresentationReceiverWindow::TitleWasSet(content::NavigationEntry* entry) {
  if (window_) {
    static_cast<WindowContents*>(window_->widget_delegate())
        ->set_title(entry->GetTitle());
  }
}

void PresentationReceiverWindow::CloseContents(content::WebContents* source) {
  presentation_web_contents_.reset();
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

void PresentationReceiverWindow::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  if (profile_ &&
      source == content::Source<Profile>(profile_->GetOriginalProfile())) {
    // Some KeyedServiceFactories that WebContents uses may call
    // |profile_->GetOriginalProfile()|.  If we aren't destroyed before the
    // original profile is being destroyed and these get called from
    // ~ProfileImpl, a CHECK will fire.  Instead we destroy the WebContents and
    // OTR profile on this notification.
    presentation_web_contents_.reset();
    profile_.reset();
  }
}

PresentationReceiverWindow::PresentationReceiverWindow(
    std::unique_ptr<Profile> window_profile,
    const GURL& start_url,
    const gfx::Rect& bounds)
    : profile_(std::move(window_profile)),
      window_(new views::Widget()),
      start_url_(start_url) {
  DCHECK(profile_);
  DCHECK(profile_->IsOffTheRecord());
  InitWindowWidgetAndViews(bounds);

  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                 content::NotificationService::AllSources());
}

void PresentationReceiverWindow::InitWindowWidgetAndViews(
    const gfx::Rect& bounds) {
  auto web_view = std::make_unique<views::WebView>(profile_.get());
  web_contents_view_ = web_view.get();

  window_->Init(CreateWidgetInitParams(
      std::make_unique<WindowContents>(this, std::move(web_view)), bounds));
}

}  // namespace media_router
