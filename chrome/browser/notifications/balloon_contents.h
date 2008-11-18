// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_BALLOON_CONTENTS_H_
#define CHROME_BROWSER_NOTIFICATIONS_BALLOON_CONTENTS_H_

#ifdef ENABLE_BACKGROUND_TASK

#include "chrome/browser/render_view_host_delegate.h"
#include "chrome/browser/views/hwnd_html_view.h"
#include "webkit/glue/webpreferences.h"

class Balloon;
class Profile;
class RenderViewHost;

class BalloonContents : public HWNDHtmlView,
                        public RenderViewHostDelegate {
 public:
  explicit BalloonContents(Balloon* balloon);

  // Stops showing the balloon.
  void Shutdown();

  // RenderViewHostDelegate implementations.
  virtual RenderViewHostDelegateType GetDelegateType() const {
    return BALLOON_CONTENTS_DELEGATE;
  }
  virtual WebPreferences GetWebkitPrefs();
  virtual SiteInstance* GetSiteInstance() const;
  virtual Profile* GetProfile() const;
  virtual void RendererReady(RenderViewHost* render_view_host);
  virtual void RendererGone(RenderViewHost* render_view_host);
  virtual void UpdateTitle(RenderViewHost* render_view_host,
                           int32 page_id,
                           const std::wstring& title);

  // Accessors.
  RenderViewHost* render_view_host() const { return render_view_host_; }
  const std::wstring& title() const { return title_; }

 private:
  // Helper functions for sending notifications.
  void NotifyConnected();
  void NotifyDisconnected();

  // The associated balloon.
  Balloon* balloon_;

  // The host HTML renderer.
  RenderViewHost* render_view_host_;

  // Indicates whether we should notify about disconnection of this balloon.
  // This is used to ensure disconnection notifications only happen if
  // a connection notification has happened and that they happen only once.
  bool notify_disconnection_;

  // The title of the balloon page.
  std::wstring title_;

  DISALLOW_COPY_AND_ASSIGN(BalloonContents);
};

#endif  // ENABLE_BACKGROUND_TASK

#endif  // CHROME_BROWSER_NOTIFICATIONS_BALLOON_CONTENTS_H_
