// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_OVERLAY_STRATEGY_FULLSCREEN_BACKGROUND_COLOR_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_OVERLAY_STRATEGY_FULLSCREEN_BACKGROUND_COLOR_H_

#include "base/macros.h"
#include "components/viz/service/display/overlay_strategy_fullscreen.h"
#include "components/viz/service/viz_service_export.h"

namespace viz {

class OverlayCandidateValidator;

// Similar to fullscreen strategy plus SolidColorDrawQuad handling.
// e.g. Youtube fullscreen consists of a video and a bunch of
// SolidColorDrawQuad.
class VIZ_SERVICE_EXPORT OverlayStrategyFullscreenBackgroundColor
    : public OverlayStrategyFullscreen {
 public:
  explicit OverlayStrategyFullscreenBackgroundColor(
      OverlayCandidateValidator* capability_checker);
  ~OverlayStrategyFullscreenBackgroundColor() override;

  bool Attempt(cc::DisplayResourceProvider* resource_provider,
               RenderPass* render_pass,
               cc::OverlayCandidateList* candidate_list,
               std::vector<gfx::Rect>* content_bounds) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OverlayStrategyFullscreenBackgroundColor);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_OVERLAY_STRATEGY_FULLSCREEN_BACKGROUND_COLOR_H_
