// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/insecure_sensitive_input_driver_factory.h"

#include <utility>
#include <vector>

#include "base/stl_util.h"
#include "chrome/browser/ssl/insecure_sensitive_input_driver.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace sensitive_input {

namespace {

const char kInsecureSensitiveInputDriverFactoryWebContentsUserDataKey[] =
    "chrome_browser_ssl_insecure_sensitive_input_driver_factory";

}  // namespace

void InsecureSensitiveInputDriverFactory::CreateForWebContents(
    content::WebContents* web_contents) {
  if (FromWebContents(web_contents))
    return;

  auto new_factory =
      base::WrapUnique(new InsecureSensitiveInputDriverFactory(web_contents));
  const std::vector<content::RenderFrameHost*> frames =
      web_contents->GetAllFrames();
  for (content::RenderFrameHost* frame : frames) {
    if (frame->IsRenderFrameLive())
      new_factory->RenderFrameCreated(frame);
  }

  web_contents->SetUserData(
      kInsecureSensitiveInputDriverFactoryWebContentsUserDataKey,
      std::move(new_factory));
}

InsecureSensitiveInputDriverFactory::InsecureSensitiveInputDriverFactory(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

InsecureSensitiveInputDriverFactory::~InsecureSensitiveInputDriverFactory() {}

// static
InsecureSensitiveInputDriverFactory*
InsecureSensitiveInputDriverFactory::FromWebContents(
    content::WebContents* contents) {
  return static_cast<InsecureSensitiveInputDriverFactory*>(
      contents->GetUserData(
          kInsecureSensitiveInputDriverFactoryWebContentsUserDataKey));
}

// static
void InsecureSensitiveInputDriverFactory::BindDriver(
    const service_manager::BindSourceInfo& source_info,
    blink::mojom::SensitiveInputVisibilityServiceRequest request,
    content::RenderFrameHost* render_frame_host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return;

  InsecureSensitiveInputDriverFactory* factory =
      InsecureSensitiveInputDriverFactory::FromWebContents(web_contents);
  if (!factory) {
    CreateForWebContents(web_contents);
    factory =
        InsecureSensitiveInputDriverFactory::FromWebContents(web_contents);
  }
  if (!factory)
    return;

  InsecureSensitiveInputDriver* driver =
      factory->GetDriverForFrame(render_frame_host);
  if (driver)
    driver->BindSensitiveInputVisibilityServiceRequest(std::move(request));
}

InsecureSensitiveInputDriver*
InsecureSensitiveInputDriverFactory::GetDriverForFrame(
    content::RenderFrameHost* render_frame_host) {
  auto mapping = frame_driver_map_.find(render_frame_host);
  return mapping == frame_driver_map_.end() ? nullptr : mapping->second.get();
}

void InsecureSensitiveInputDriverFactory::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  auto insertion_result =
      frame_driver_map_.insert(std::make_pair(render_frame_host, nullptr));
  // This is called twice for the main frame.

  if (insertion_result.second) {  // This was the first time.
    insertion_result.first->second =
        base::MakeUnique<InsecureSensitiveInputDriver>(render_frame_host);
  }
}

void InsecureSensitiveInputDriverFactory::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  frame_driver_map_.erase(render_frame_host);
}

}  // namespace sensitive_input
