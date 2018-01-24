// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CONTEXTUALSEARCH_UNHANDLED_TAP_WEB_CONTENTS_OBSERVER_H_
#define CHROME_BROWSER_ANDROID_CONTEXTUALSEARCH_UNHANDLED_TAP_WEB_CONTENTS_OBSERVER_H_

#include "base/macros.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace contextual_search {

typedef base::RepeatingCallback<void(int x_px, int y_px)> UnhandledTapCallback;

class UnhandledTapWebContentsObserver : public content::WebContentsObserver {
 public:
  UnhandledTapWebContentsObserver(content::WebContents* web_contents,
                                  float device_scale_factor,
                                  UnhandledTapCallback callback);

  ~UnhandledTapWebContentsObserver() override;

 private:
  // content::WebContentsObserver implementation.
  void OnInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;

  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(UnhandledTapWebContentsObserver);
};

}  // namespace contextual_search

#endif  // CHROME_BROWSER_ANDROID_CONTEXTUALSEARCH_UNHANDLED_TAP_WEB_CONTENTS_OBSERVER_H_
