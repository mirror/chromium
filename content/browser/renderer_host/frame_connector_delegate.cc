// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/frame_connector_delegate.h"

#include "content/common/content_switches_internal.h"

namespace content {

FrameConnectorDelegate::~FrameConnectorDelegate() {}

RenderWidgetHostViewBase*
FrameConnectorDelegate::GetParentRenderWidgetHostView() {
  return nullptr;
}

RenderWidgetHostViewBase*
FrameConnectorDelegate::GetRootRenderWidgetHostView() {
  return nullptr;
}

gfx::PointF FrameConnectorDelegate::TransformPointToRootCoordSpace(
    const gfx::PointF& point,
    const viz::SurfaceId& surface_id) {
  return gfx::PointF();
}

bool FrameConnectorDelegate::TransformPointToLocalCoordSpace(
    const gfx::PointF& point,
    const viz::SurfaceId& original_surface,
    const viz::SurfaceId& local_surface_id,
    gfx::PointF* transformed_point) {
  return false;
}

bool FrameConnectorDelegate::TransformPointToCoordSpaceForView(
    const gfx::PointF& point,
    RenderWidgetHostViewBase* target_view,
    const viz::SurfaceId& local_surface_id,
    gfx::PointF* transformed_point) {
  return false;
}

bool FrameConnectorDelegate::HasFocus() {
  return false;
}

bool FrameConnectorDelegate::LockMouse() {
  return false;
}

bool FrameConnectorDelegate::IsInert() const {
  return false;
}

bool FrameConnectorDelegate::IsHidden() const {
  return false;
}

bool FrameConnectorDelegate::IsThrottled() const {
  return false;
}

bool FrameConnectorDelegate::IsSubtreeThrottled() const {
  return false;
}

void FrameConnectorDelegate::SetRect(const gfx::Rect& frame_rect) {
  if (use_zoom_for_device_scale_factor_) {
    frame_rects_.emplace(
        gfx::ScaleToEnclosingRect(frame_rect,
                                  1.f / screen_info_.device_scale_factor),
        frame_rect);
  } else {
    frame_rects_.emplace(frame_rect,
                         gfx::ScaleToEnclosingRect(
                             frame_rect, screen_info_.device_scale_factor));
  }
}

FrameConnectorDelegate::FrameConnectorDelegate(
    bool use_zoom_for_device_scale_factor)
    : use_zoom_for_device_scale_factor_(use_zoom_for_device_scale_factor) {}

}  // namespace content
