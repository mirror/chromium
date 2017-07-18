// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_INSECURE_SENSITIVE_INPUT_DRIVER_FACTORY_H_
#define CHROME_BROWSER_SSL_INSECURE_SENSITIVE_INPUT_DRIVER_FACTORY_H_

#include <map>
#include <memory>

#include "base/supports_user_data.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "third_party/WebKit/public/platform/modules/sensitive_input_visibility/sensitive_input_visibility_service.mojom.h"

namespace content {
class WebContents;
}

namespace sensitive_input {

class InsecureSensitiveInputDriver;

// Creates and owns InsecureSensitiveInputDrivers. There is one
// factory per WebContents, and one driver per render frame.
class InsecureSensitiveInputDriverFactory
    : public content::WebContentsObserver,
      public base::SupportsUserData::Data {
 public:
  static void CreateForWebContents(content::WebContents* web_contents);
  ~InsecureSensitiveInputDriverFactory() override;

  static InsecureSensitiveInputDriverFactory* FromWebContents(
      content::WebContents* web_contents);

  static void BindDriver(
      const service_manager::BindSourceInfo& source_info,
      blink::mojom::SensitiveInputVisibilityServiceRequest request,
      content::RenderFrameHost* render_frame_host);

  InsecureSensitiveInputDriver* GetDriverForFrame(
      content::RenderFrameHost* render_frame_host);

  // content::WebContentsObserver:
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;

 private:
  explicit InsecureSensitiveInputDriverFactory(
      content::WebContents* web_contents);

  std::map<content::RenderFrameHost*,
           std::unique_ptr<InsecureSensitiveInputDriver>>
      frame_driver_map_;

  DISALLOW_COPY_AND_ASSIGN(InsecureSensitiveInputDriverFactory);
};

}  // namespace sensitive_input

#endif  // CHROME_BROWSER_SSL_INSECURE_SENSITIVE_INPUT_DRIVER_FACTORY_H_
