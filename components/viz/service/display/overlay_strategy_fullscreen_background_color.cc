// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/overlay_strategy_fullscreen_background_color.h"

#include "base/bind.h"
#include "base/containers/adapters.h"
#include "components/viz/common/quads/draw_quad.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "ui/gfx/geometry/rect_conversions.h"

namespace viz {

namespace {

bool IsOnBlackColorQuads(QuadList::ConstIterator quad_list_begin,
                         QuadList::ConstIterator quad_list_end) {
  for (auto it = quad_list_begin; it != quad_list_end; ++it) {
    const DrawQuad* back_quad = *it;
    if (back_quad->material != DrawQuad::SOLID_COLOR) {
      return false;
    }

    // TODO(dshwang): support other background color. kms atomic modeset will
    // support background color, which currently is not exposed and assumed to
    // be black.
    const SolidColorDrawQuad* color_quad =
        SolidColorDrawQuad::MaterialCast(back_quad);
    if (SK_ColorBLACK != color_quad->color) {
      return false;
    }
  }

  return true;
}

}  // namespace

OverlayStrategyFullscreenBackgroundColor::
    OverlayStrategyFullscreenBackgroundColor(
        OverlayCandidateValidator* capability_checker)
    : OverlayStrategyFullscreen(capability_checker) {
  OverlayStrategyFullscreen::SetQuadListCheckerCallback(
      base::Bind(&IsOnBlackColorQuads));
}

OverlayStrategyFullscreenBackgroundColor::
    ~OverlayStrategyFullscreenBackgroundColor() {}

bool OverlayStrategyFullscreenBackgroundColor::Attempt(
    cc::DisplayResourceProvider* resource_provider,
    RenderPass* render_pass,
    cc::OverlayCandidateList* candidate_list,
    std::vector<gfx::Rect>* content_bounds) {
  return OverlayStrategyFullscreen::Attempt(resource_provider, render_pass,
                                            candidate_list, content_bounds);
}

}  // namespace viz
