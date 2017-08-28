// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CONTENT_RENDERER_DISTILLABILITY_WEBAGENT_H_
#define COMPONENTS_DOM_DISTILLER_CONTENT_RENDERER_DISTILLABILITY_WEBAGENT_H_

#include "content/public/renderer/render_frame_observer.h"
#include "third_party/WebKit/webagents/agent.h"

namespace dom_distiller {

// DistillabilityWebagent returns distillability result to DistillabilityDriver.
class DistillabilityWebagent : public content::RenderFrameObserver,
                               public webagents::Agent {
 public:
  DistillabilityWebagent(content::RenderFrame* render_frame);
  ~DistillabilityWebagent() override;

  // content::RenderFrameObserver:
  void DidMeaningfulLayout(blink::WebMeaningfulLayout) override;
  void OnDestruct() override;
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CONTENT_RENDERER_DISTILLABILITY_WEBAGENT_H_
