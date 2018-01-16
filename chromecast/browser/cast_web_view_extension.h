// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_WEB_VIEW_EXTENSION_H_
#define CHROMECAST_BROWSER_CAST_WEB_VIEW_EXTENSION_H_

#include "chromecast/browser/cast_web_view.h"
#include "content/public/browser/media_capture_devices.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/renderer_preferences.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"
#include "url/gurl.h"

namespace chromecast {

class CastExtensionHost;
class CastWebContentsManager;

// A simplified interface for loading and displaying WebContents in cast_shell.
class CastWebViewExtension : public CastWebView {
 public:
  // |delegate| and |browser_context| should outlive the lifetime of this
  // object.
  CastWebViewExtension(const extensions::Extension* extension,
                       const GURL& initial_url,
                       CastWebView::Delegate* delegate,
                       CastWebContentsManager* web_contents_manager,
                       content::BrowserContext* browser_context,
                       scoped_refptr<content::SiteInstance> site_instance,
                       bool transparent,
                       bool allow_media_access,
                       bool is_headless,
                       bool enable_touch_input);
  ~CastWebViewExtension() override;

  shell::CastContentWindow* window() const override;

  content::WebContents* web_contents() const override;

  // Navigates to |url|. The loaded page will be preloaded if MakeVisible has
  // not been called on the object.
  void LoadUrl(GURL url) override;

  // Begins the close process for this page (ie. triggering document.onunload).
  // A consumer of the class can be notified when the process has been finished
  // via Delegate::OnPageStopped(). The page will be torn down after
  // |shutdown_delay| has elapsed, or sooner if required.
  void ClosePage(const base::TimeDelta& shutdown_delay) override;

  // Makes the page visible to the user.
  void Show(CastWindowManager* window_manager) override;

 private:
  std::unique_ptr<shell::CastContentWindow> window_;

  std::unique_ptr<CastExtensionHost> extension_host_;
  scoped_refptr<content::SiteInstance> site_instance_;

  base::WeakPtrFactory<CastWebViewExtension> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CastWebViewExtension);
};

}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_CAST_WEB_VIEW_EXTENSION_H_
