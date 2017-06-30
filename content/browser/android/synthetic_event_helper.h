// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_SYNTHETIC_EVENT_HELPER_H_
#define CONTENT_BROWSER_ANDROID_SYNTHETIC_EVENT_HELPER_H_

#include "content/browser/android/render_widget_host_connector.h"
#include "content/common/content_export.h"
#include "content/public/browser/android/motion_event_synthesizer.h"

namespace content {
class WebContents;

// Helper class that provides access to WebContents for MotionEventSynthesizer
// to manage its Java layer. Owns itself, and gets destroyed upon
// |WebContentsObserver::WebContentsDestroyed|.
class SyntheticEventHelper : public RenderWidgetHostConnector,
                             public MotionEventSynthesizer::Helper {
 public:
  explicit SyntheticEventHelper(WebContents* web_contents);

  // MotionEventSynthesizer::Helper.
  WebContents* GetWebContents() const override;

  // RendetWidgetHostConnector implementation.
  void UpdateRenderProcessConnection(
      RenderWidgetHostViewAndroid* old_rwhva,
      RenderWidgetHostViewAndroid* new_rhwva) override;

 private:
  WebContents* const web_contents_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_SYNTHETIC_EVENT_HELPER_H_
