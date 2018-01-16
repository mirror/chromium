// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_EXTENSION_HOST_H_
#define CHROMECAST_BROWSER_CAST_EXTENSION_HOST_H_

#include "chromecast/browser/cast_web_view.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"
#include "url/gurl.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

namespace chromecast {

class CastExtensionHost : public extensions::ExtensionHost,
                          public content::NotificationObserver {
 public:
  CastExtensionHost(content::BrowserContext* browser_context,
                    CastWebView::Delegate* delegate,
                    const extensions::Extension* extension,
                    const GURL& initial_url,
                    content::SiteInstance* site_instance,
                    extensions::ViewType host_type);
  ~CastExtensionHost() override;

  // extensions::ExtensionHost:
  bool IsBackgroundPage() const override;
  void OnDidStopFirstLoad() override;
  void LoadInitialURL() override;
  void ActivateContents(content::WebContents* contents) override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void LoadingStateChanged(content::WebContents* source,
                           bool to_different_document) override;

 private:
  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  content::NotificationRegistrar registrar_;
  content::BrowserContext* browser_context_;
  CastWebView::Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(CastExtensionHost);
};

}  // namespace chromecast

#endif
