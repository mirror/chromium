// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_HELPER_H_
#define CHROME_BROWSER_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_HELPER_H_

#include <memory>

#include "chrome/browser/chromeos/test_system_app/test_system_app.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {
class RenderFrameHost;
class RenderViewHost;
class WebContents;
}

class SystemWebAppMojoHandler : public content::WebContentsObserver {
 public:
  explicit SystemWebAppMojoHandler(content::WebContents* contents);
  ~SystemWebAppMojoHandler() override;

 protected:
  // content::WebContentsObserver implementation.
  void OnInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;

  void BindTestSystemApp(mojo::InterfaceRequest<mojom::TestSystemApp> request);

 private:
  service_manager::BinderRegistry registry_;
  std::unique_ptr<mojom::TestSystemApp> test_system_app_;

  DISALLOW_COPY_AND_ASSIGN(SystemWebAppMojoHandler);
};

class SystemWebAppHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SystemWebAppHelper> {
 public:
  ~SystemWebAppHelper() override;

  // content::WebContentsObserver implementation.
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;

 private:
  explicit SystemWebAppHelper(content::WebContents* contents);

  friend class content::WebContentsUserData<SystemWebAppHelper>;

  std::unique_ptr<SystemWebAppMojoHandler> mojo_handler_;
};

#endif  // CHROME_BROWSER_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_HELPER_H_
