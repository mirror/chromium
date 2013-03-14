// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_dfb.h"

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/time.h"
#include "content/browser/renderer_host/backing_store_dfb.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_context.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebScreenInfo.h"

using WebKit::WebMouseWheelEvent;
using WebKit::WebScreenInfo;

namespace content {
namespace {

// Paint rects on Linux are bounded by the maximum size of a shared memory
// region. By default that's 32MB, but many distros increase it significantly
// (i.e. to 256MB).
//
// We fetch the maximum value from /proc/sys/kernel/shmmax at runtime and, if
// we exceed that, then we limit the height of the paint rect in the renderer.
//
// These constants are here to ensure that, in the event that we exceed it, we
// end up with something a little more square. Previously we had 4000x4000, but
// people's monitor setups are actually exceeding that these days.
const int kMaxWindowWidth = 10000;
const int kMaxWindowHeight = 10000;

bool MovedToPoint(const WebKit::WebMouseEvent& mouse_event,
                   const gfx::Point& center) {
  return mouse_event.globalX == center.x() &&
         mouse_event.globalY == center.y();
}

}  // namespace

RenderWidgetHostViewDfb::RenderWidgetHostViewDfb(RenderWidgetHost* widget_host)
    : host_(RenderWidgetHostImpl::From(widget_host)),
      about_to_validate_and_paint_(false),
      is_hidden_(false),
      is_loading_(false),
      is_popup_first_mouse_release_(true),
      is_fullscreen_(false),
      made_active_(false),
      mouse_is_being_warped_to_unlocked_position_(false),
      dragged_at_horizontal_edge_(0),
      dragged_at_vertical_edge_(0) {
  host_->SetView(this);
}

RenderWidgetHostViewDfb::~RenderWidgetHostViewDfb() {
  UnlockMouse();
}

void RenderWidgetHostViewDfb::InitAsChild(
    gfx::NativeView parent_view) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::InitAsPopup(
    RenderWidgetHostView* parent_host_view, const gfx::Rect& pos) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::InitAsFullscreen(
    RenderWidgetHostView* reference_host_view) {
  is_fullscreen_ = true;
  NOTIMPLEMENTED();
}

RenderWidgetHost* RenderWidgetHostViewDfb::GetRenderWidgetHost() const {
  return host_;
}

void RenderWidgetHostViewDfb::WasShown() {
  if (!is_hidden_)
    return;

  if (web_contents_switch_paint_time_.is_null())
    web_contents_switch_paint_time_ = base::TimeTicks::Now();
  is_hidden_ = false;
  host_->WasShown();
}

void RenderWidgetHostViewDfb::WasHidden() {
  if (is_hidden_)
    return;

  // If we receive any more paint messages while we are hidden, we want to
  // ignore them so we don't re-allocate the backing store.  We will paint
  // everything again when we become selected again.
  is_hidden_ = true;

  // If we have a renderer, then inform it that we are being hidden so it can
  // reduce its resource utilization.
  host_->WasHidden();
}

void RenderWidgetHostViewDfb::SetSize(const gfx::Size& size) {
  int width = std::min(size.width(), kMaxWindowWidth);
  int height = std::min(size.height(), kMaxWindowHeight);

  // Update the size of the RWH.
  if (requested_size_.width() != width ||
      requested_size_.height() != height) {
    requested_size_ = gfx::Size(width, height);
    host_->SendScreenRects();
    host_->WasResized();
  }
}

void RenderWidgetHostViewDfb::SetBounds(const gfx::Rect& rect) {
  // This is called when webkit has sent us a Move message.
  SetSize(rect.size());
}

gfx::NativeView RenderWidgetHostViewDfb::GetNativeView() const {
  NOTIMPLEMENTED();
  return NULL;
}

gfx::NativeViewId RenderWidgetHostViewDfb::GetNativeViewId() const {
  NOTIMPLEMENTED();
  return 0;
}

gfx::NativeViewAccessible RenderWidgetHostViewDfb::GetNativeViewAccessible() {
  NOTIMPLEMENTED();
  return NULL;
}

void RenderWidgetHostViewDfb::MovePluginWindows(
    const gfx::Vector2d& scroll_offset,
    const std::vector<webkit::npapi::WebPluginGeometry>& moves) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::Focus() {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::Blur() {
  // TODO(estade): We should be clearing native focus as well, but I know of no
  // way to do that without focusing another widget.
  host_->Blur();
}

bool RenderWidgetHostViewDfb::HasFocus() const {
  NOTIMPLEMENTED() << "Bogus value return";
  return true;
}

bool RenderWidgetHostViewDfb::IsSurfaceAvailableForCopy() const {
  return !!host_->GetBackingStore(false);
}

void RenderWidgetHostViewDfb::Show() {
  is_hidden_ = false;
}

void RenderWidgetHostViewDfb::Hide() {
  is_hidden_ = true;
}

bool RenderWidgetHostViewDfb::IsShowing() {
  return !!is_hidden_;
}

gfx::Rect RenderWidgetHostViewDfb::GetViewBounds() const {
  return gfx::Rect(0, 0, requested_size_.width(), requested_size_.height());
}

void RenderWidgetHostViewDfb::UpdateCursor(const WebCursor& cursor) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::SetIsLoading(bool is_loading) {
  is_loading_ = is_loading;
}

void RenderWidgetHostViewDfb::TextInputStateChanged(
    const ViewHostMsg_TextInputState_Params& params) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::ImeCancelComposition() {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::ImeCompositionRangeChanged(
    const ui::Range& range,
    const std::vector<gfx::Rect>& character_bounds) {
}

void RenderWidgetHostViewDfb::DidUpdateBackingStore(
    const gfx::Rect& scroll_rect,
    const gfx::Vector2d& scroll_delta,
    const std::vector<gfx::Rect>& copy_rects) {
  TRACE_EVENT0("ui::gtk", "RenderWidgetHostViewDfb::DidUpdateBackingStore");

  if (is_hidden_)
    return;

  // TODO(darin): Implement the equivalent of Win32's ScrollWindowEX.  Can that
  // be done using XCopyArea?  Perhaps similar to
  // BackingStore::ScrollBackingStore?
  if (about_to_validate_and_paint_)
    invalid_rect_.Union(scroll_rect);
  else
    Paint(scroll_rect);

  for (size_t i = 0; i < copy_rects.size(); ++i) {
    // Avoid double painting.  NOTE: This is only relevant given the call to
    // Paint(scroll_rect) above.
    gfx::Rect rect = gfx::SubtractRects(copy_rects[i], scroll_rect);
    if (rect.IsEmpty())
      continue;

    if (about_to_validate_and_paint_)
      invalid_rect_.Union(rect);
    else
      Paint(rect);
  }
}

void RenderWidgetHostViewDfb::RenderViewGone(base::TerminationStatus status,
                                             int error_code) {
  Destroy();
}

void RenderWidgetHostViewDfb::Destroy() {
  // The RenderWidgetHost's destruction led here, so don't call it.
  host_ = NULL;

  MessageLoop::current()->DeleteSoon(FROM_HERE, this);
}

void RenderWidgetHostViewDfb::SetTooltipText(const string16& tooltip_text) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::SelectionBoundsChanged(
    const ViewHostMsg_SelectionBounds_Params& params) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::ScrollOffsetChanged() {
}

BackingStore* RenderWidgetHostViewDfb::AllocBackingStore(
    const gfx::Size& size) {
	return new BackingStoreDfb(host_, size);
}

// NOTE: |output| is initialized with the size of |src_subrect|, and |dst_size|
// is ignored on GTK.
void RenderWidgetHostViewDfb::CopyFromCompositingSurface(
    const gfx::Rect& src_subrect,
    const gfx::Size& /* dst_size */,
    const base::Callback<void(bool, const SkBitmap&)>& callback) {
  SkBitmap bitmap;
  NOTIMPLEMENTED();
  callback.Run(false, bitmap);
}

void RenderWidgetHostViewDfb::CopyFromCompositingSurfaceToVideoFrame(
      const gfx::Rect& src_subrect,
      const scoped_refptr<media::VideoFrame>& target,
      const base::Callback<void(bool)>& callback) {
  NOTIMPLEMENTED();
  callback.Run(false);
}

bool RenderWidgetHostViewDfb::CanCopyToVideoFrame() const {
  return false;
}

void RenderWidgetHostViewDfb::AcceleratedSurfaceBuffersSwapped(
    const GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params& params,
    int gpu_host_id) {
   AcceleratedSurfaceMsg_BufferPresented_Params ack_params;
   ack_params.sync_point = 0;
   RenderWidgetHostImpl::AcknowledgeBufferPresent(
      params.route_id, gpu_host_id, ack_params);
}

void RenderWidgetHostViewDfb::AcceleratedSurfacePostSubBuffer(
    const GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params& params,
    int gpu_host_id) {
   AcceleratedSurfaceMsg_BufferPresented_Params ack_params;
   ack_params.sync_point = 0;
   RenderWidgetHostImpl::AcknowledgeBufferPresent(
      params.route_id, gpu_host_id, ack_params);
}

void RenderWidgetHostViewDfb::AcceleratedSurfaceSuspend() {
}

void RenderWidgetHostViewDfb::AcceleratedSurfaceRelease() {
}

bool RenderWidgetHostViewDfb::HasAcceleratedSurface(
      const gfx::Size& desired_size) {
  // TODO(jbates) Implement this so this view can use GetBackingStore for both
  // software and GPU frames. Defaulting to false just makes GetBackingStore
  // only useable for software frames.
  return false;
}

void RenderWidgetHostViewDfb::SetBackground(const SkBitmap& background) {
  RenderWidgetHostViewBase::SetBackground(background);
  host_->Send(new ViewMsg_SetBackground(host_->GetRoutingID(), background));
}

void RenderWidgetHostViewDfb::Paint(const gfx::Rect& damage_rect) {
  TRACE_EVENT0("ui::dfb", "RenderWidgetHostViewDfb::Paint");

  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::ShowCurrentCursor() {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::SetHasHorizontalScrollbar(
    bool has_horizontal_scrollbar) {
}

void RenderWidgetHostViewDfb::SetScrollOffsetPinning(
    bool is_pinned_to_left, bool is_pinned_to_right) {
}


void RenderWidgetHostViewDfb::OnAcceleratedCompositingStateChange() {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::GetScreenInfo(WebScreenInfo* results) {
  NOTIMPLEMENTED();
}

gfx::Rect RenderWidgetHostViewDfb::GetBoundsInRootWindow() {
  NOTIMPLEMENTED() << "Bogus size returned";
  return gfx::Rect(0, 0, 1368, 768);
}

gfx::GLSurfaceHandle RenderWidgetHostViewDfb::GetCompositingSurface() {
  NOTIMPLEMENTED();
  return gfx::GLSurfaceHandle();
}

bool RenderWidgetHostViewDfb::LockMouse() {
  NOTIMPLEMENTED();
  return false;
}

void RenderWidgetHostViewDfb::UnlockMouse() {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewDfb::OnAccessibilityNotifications(
    const std::vector<AccessibilityHostMsg_NotificationParams>& params) {
  NOTIMPLEMENTED();
}


void RenderWidgetHostViewDfb::ModifyEventMovementAndCoords(
    WebKit::WebMouseEvent* event) {
  // Movement is computed by taking the difference of the new cursor position
  // and the previous. Under mouse lock the cursor will be warped back to the
  // center so that we are not limited by clipping boundaries.
  // We do not measure movement as the delta from cursor to center because
  // we may receive more mouse movement events before our warp has taken
  // effect.
  event->movementX = event->globalX - global_mouse_position_.x();
  event->movementY = event->globalY - global_mouse_position_.y();

  // While the cursor is being warped back to the unlocked position, suppress
  // the movement member data.
  if (mouse_is_being_warped_to_unlocked_position_) {
    event->movementX = 0;
    event->movementY = 0;
    if (MovedToPoint(*event, unlocked_global_mouse_position_))
      mouse_is_being_warped_to_unlocked_position_ = false;
  }

  global_mouse_position_.SetPoint(event->globalX, event->globalY);

  // Under mouse lock, coordinates of mouse are locked to what they were when
  // mouse lock was entered.
  if (mouse_locked_) {
    event->x = unlocked_mouse_position_.x();
    event->y = unlocked_mouse_position_.y();
    event->windowX = unlocked_mouse_position_.x();
    event->windowY = unlocked_mouse_position_.y();
    event->globalX = unlocked_global_mouse_position_.x();
    event->globalY = unlocked_global_mouse_position_.y();
  } else {
    unlocked_mouse_position_.SetPoint(event->windowX, event->windowY);
    unlocked_global_mouse_position_.SetPoint(event->globalX, event->globalY);
  }
}

////////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostView, public:

// static
RenderWidgetHostView* RenderWidgetHostView::CreateViewForWidget(
    RenderWidgetHost* widget) {
  return new RenderWidgetHostViewDfb(widget);
}

// static
void RenderWidgetHostViewPort::GetDefaultScreenInfo(WebScreenInfo* results) {
  // TODO(msb): fix the hard coded values.

  results->deviceScaleFactor = 1.0;
  results->depthPerComponent = 8;
  results->depth = 8;
  results->isMonochrome = false;
  results->availableRect = results->rect = WebKit::WebRect(0, 0, 1920, 1080);
  NOTIMPLEMENTED() << "Bogus value return";
}

}  // namespace content
