// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/render_widget_host_view_mus_guest.h"

namespace content {

// NOTE: RenderWidgetHostViewMusGuest is *not* set as the RenderWidgetHostView
// of a RenderWidgetHost, rather RenderWidgetHostViewGuest delegates a handful
// of functions to RenderWidgetHostViewMusGuest (by way of
// |RenderWidgetHostViewGuest::platform_view_|). Because of this, many of the
// functions here are not called at all and are NOTREACHED.

RenderWidgetHostViewMusGuest::RenderWidgetHostViewMusGuest() {}

RenderWidgetHostViewMusGuest::~RenderWidgetHostViewMusGuest() {}

void RenderWidgetHostViewMusGuest::InitAsChild(gfx::NativeView parent_view) {
  NOTREACHED();
}

void RenderWidgetHostViewMusGuest::SetSize(const gfx::Size& size) {
  NOTREACHED();
}

void RenderWidgetHostViewMusGuest::SetBounds(const gfx::Rect& rect) {
  NOTREACHED();
}

gfx::Vector2dF RenderWidgetHostViewMusGuest::GetLastScrollOffset() const {
  NOTREACHED();
  return gfx::Vector2dF();
}

gfx::NativeView RenderWidgetHostViewMusGuest::GetNativeView() const {
  NOTREACHED();
  return nullptr;
}

gfx::NativeViewAccessible
RenderWidgetHostViewMusGuest::GetNativeViewAccessible() {
  NOTREACHED();
  return nullptr;
}

void RenderWidgetHostViewMusGuest::Focus() {
  NOTREACHED();
}

bool RenderWidgetHostViewMusGuest::HasFocus() const {
  NOTREACHED();
  return false;
}

void RenderWidgetHostViewMusGuest::Show() {
  NOTREACHED();
}

void RenderWidgetHostViewMusGuest::Hide() {
  NOTREACHED();
}

bool RenderWidgetHostViewMusGuest::IsShowing() {
  NOTREACHED();
  return false;
}

gfx::Rect RenderWidgetHostViewMusGuest::GetViewBounds() const {
  NOTREACHED();
  return gfx::Rect();
}

void RenderWidgetHostViewMusGuest::SetBackgroundColor(SkColor color) {
  NOTREACHED();
}

SkColor RenderWidgetHostViewMusGuest::background_color() const {
  NOTREACHED();
  return SK_ColorWHITE;
}

void RenderWidgetHostViewMusGuest::SetNeedsBeginFrames(
    bool needs_begin_frames) {
  NOTREACHED();
}

void RenderWidgetHostViewMusGuest::SubmitCompositorFrame(
    const viz::LocalSurfaceId& local_surface_id,
    viz::CompositorFrame frame) {
  NOTREACHED();
}

void RenderWidgetHostViewMusGuest::ClearCompositorFrame() {
  NOTREACHED();
}
void RenderWidgetHostViewMusGuest::InitAsPopup(
    RenderWidgetHostView* parent_host_view,
    const gfx::Rect& bounds) {
  NOTREACHED();
}

void RenderWidgetHostViewMusGuest::InitAsFullscreen(
    RenderWidgetHostView* reference_host_view) {
  NOTREACHED();
}

void RenderWidgetHostViewMusGuest::UpdateCursor(const WebCursor& cursor) {
  NOTREACHED();
}

void RenderWidgetHostViewMusGuest::SetTooltipText(
    const base::string16& tooltip_text) {
  NOTREACHED();
}

bool RenderWidgetHostViewMusGuest::HasAcceleratedSurface(
    const gfx::Size& desired_size) {
  NOTREACHED();
  return false;
}

gfx::Rect RenderWidgetHostViewMusGuest::GetBoundsInRootWindow() {
  NOTREACHED();
  return gfx::Rect();
}

void RenderWidgetHostViewMusGuest::RenderProcessGone(
    base::TerminationStatus status,
    int error_code) {
  Destroy();
}

void RenderWidgetHostViewMusGuest::Destroy() {
  delete this;
}

base::string16 RenderWidgetHostViewMusGuest::GetSelectedText() {
  return RenderWidgetHostViewBase::GetSelectedText();
}

void RenderWidgetHostViewMusGuest::SetIsLoading(bool is_loading) {}

void RenderWidgetHostViewMusGuest::SelectionChanged(const base::string16& text,
                                                    size_t offset,
                                                    const gfx::Range& range) {
  return RenderWidgetHostViewBase::SelectionChanged(text, offset, range);
}

bool RenderWidgetHostViewMusGuest::LockMouse() {
  // TODO(sky): figure out what to do here.
  NOTIMPLEMENTED();
  return false;
}

void RenderWidgetHostViewMusGuest::UnlockMouse() {
  // TODO(sky): figure out what to do here.
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewMusGuest::DidCreateNewRendererCompositorFrameSink(
    viz::mojom::CompositorFrameSinkClient* renderer_compositor_frame_sink) {}

}  // namespace content
