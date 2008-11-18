// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#include "chrome/browser/notifications/balloon_contents.h"

#include "chrome/browser/browser_list.h"
#include "chrome/browser/notifications/balloons.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/render_view_host.h"
#include "chrome/browser/render_widget_host_view_win.h"
#include "chrome/browser/site_instance.h"
#include "chrome/common/notification_types.h"

BalloonContents::BalloonContents(Balloon* balloon)
    : HWNDHtmlView(balloon->notification().url(), this, true),
      balloon_(balloon),
      render_view_host_(NULL),
      notify_disconnection_(false) {
  DCHECK(balloon);
}

void BalloonContents::Shutdown() {
  if (render_view_host_) {
    render_view_host_->Shutdown();
    render_view_host_ = NULL;
  }
}

WebPreferences BalloonContents::GetWebkitPrefs() {
  return WebPreferences();
}

SiteInstance* BalloonContents::GetSiteInstance() const {
  return balloon_->site_instance();
}

Profile* BalloonContents::GetProfile() const {
  return balloon_->profile();
}

void BalloonContents::RendererReady(RenderViewHost* render_view_host) {
  NotifyConnected();
}

void BalloonContents::RendererGone(RenderViewHost* render_view_host) {
  NotifyDisconnected();
}

void BalloonContents::UpdateTitle(RenderViewHost* render_view_host,
                                  int32 page_id,
                                  const std::wstring& title) {
  title_ = title;
}

void BalloonContents::NotifyConnected() {
  notify_disconnection_ = true;
  NotificationService::current()->Notify(NOTIFY_BALLOON_CONNECTED,
      Source<Balloon>(balloon_),
      NotificationService::NoDetails());
}

void BalloonContents::NotifyDisconnected() {
  if (!notify_disconnection_) {
    return;
  }
  notify_disconnection_ = false;
  NotificationService::current()->Notify(NOTIFY_BALLOON_DISCONNECTED,
      Source<Balloon>(balloon_),
      NotificationService::NoDetails());
}

#endif  // ENABLE_BACKGROUND_TASK
