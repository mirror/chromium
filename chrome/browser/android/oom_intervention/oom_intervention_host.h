// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_HOST_H_
#define CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_HOST_H_

#include "chrome/browser/android/oom_intervention/near_oom_monitor.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/WebKit/public/platform/oom_intervention.mojom.h"

namespace content {
class WebContents;
}

class OomInterventionHost
    : public content::WebContentsObserver,
      public content::WebContentsUserData<OomInterventionHost>,
      public NearOomMonitor::Observer {
 public:
  OomInterventionHost(content::WebContents* web_contents);
  ~OomInterventionHost() override;

  void AcceptIntervention();
  void DeclineIntervention();

 private:
  friend class content::WebContentsUserData<OomInterventionHost>;

  // content::WebContentsObserver:
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void WebContentsDestroyed() override;

  // NearOomMonitor::Observer:
  void OnNearOomDetected() override;

  bool suspended_;
  blink::mojom::OomInterventionPtr oom_intervention_;
};

#endif  // CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_HOST_H_
