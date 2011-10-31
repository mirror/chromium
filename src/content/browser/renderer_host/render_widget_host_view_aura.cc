// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_aura.h"

#include "base/logging.h"
#include "content/browser/renderer_host/backing_store_skia.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/web_input_event_aura.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "ui/aura/aura_constants.h"
#include "ui/aura/desktop.h"
#include "ui/aura/event.h"
#include "ui/aura/hit_test.h"
#include "ui/aura/window.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/compositor/layer.h"

#if defined(UI_COMPOSITOR_IMAGE_TRANSPORT)
#include "base/bind.h"
#include "content/browser/gpu/gpu_process_host_ui_shim.h"
#include "content/browser/renderer_host/accelerated_surface_container_linux.h"
#include "content/common/gpu/gpu_messages.h"
#include "ui/gfx/gl/gl_bindings.h"
#endif

using WebKit::WebTouchEvent;

namespace {

#if defined(UI_COMPOSITOR_IMAGE_TRANSPORT)
void AcknowledgeSwapBuffers(int32 route_id, int gpu_host_id) {
  // It's possible that gpu_host_id is no longer valid at this point (like if
  // gpu process was restarted after a crash).  SendToGpuHost handles this.
  GpuProcessHostUIShim::SendToGpuHost(gpu_host_id,
      new AcceleratedSurfaceMsg_BuffersSwappedACK(route_id));
}
#endif

ui::TouchStatus DecideTouchStatus(const WebKit::WebTouchEvent& event,
                                  WebKit::WebTouchPoint* point) {
  if (event.type == WebKit::WebInputEvent::TouchStart &&
      event.touchesLength == 1)
    return ui::TOUCH_STATUS_START;

  if (event.type == WebKit::WebInputEvent::TouchMove && point == NULL)
    return ui::TOUCH_STATUS_CONTINUE;

  if (event.type == WebKit::WebInputEvent::TouchEnd &&
      event.touchesLength == 0)
    return ui::TOUCH_STATUS_END;

  if (event.type == WebKit::WebInputEvent::TouchCancel)
    return ui::TOUCH_STATUS_CANCEL;

  return point ? ui::TOUCH_STATUS_CONTINUE : ui::TOUCH_STATUS_UNKNOWN;
}

void UpdateWebTouchEventAfterDispatch(WebKit::WebTouchEvent* event,
                                      WebKit::WebTouchPoint* point) {
  if (point->state != WebKit::WebTouchPoint::StateReleased)
    return;
  --event->touchesLength;
  for (unsigned i = point - event->touches;
       i < event->touchesLength;
       ++i) {
    event->touches[i] = event->touches[i + 1];
  }
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostViewAura, public:

RenderWidgetHostViewAura::RenderWidgetHostViewAura(RenderWidgetHost* host)
    : host_(host),
      ALLOW_THIS_IN_INITIALIZER_LIST(window_(new aura::Window(this))),
      is_loading_(false) {
  host_->SetView(this);
  window_->SetProperty(aura::kTooltipTextKey, &tooltip_);
}

RenderWidgetHostViewAura::~RenderWidgetHostViewAura() {
}

void RenderWidgetHostViewAura::Init() {
  window_->Init(ui::Layer::LAYER_HAS_TEXTURE);
}

////////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostViewAura, RenderWidgetHostView implementation:

void RenderWidgetHostViewAura::InitAsPopup(
    RenderWidgetHostView* parent_host_view,
    const gfx::Rect& pos) {
  NOTIMPLEMENTED();
  // TODO(ivankr): there has to be an Init() call, otherwise |window_|
  // is left uninitialized and will eventually crash.
  Init();
}

void RenderWidgetHostViewAura::InitAsFullscreen(
    RenderWidgetHostView* reference_host_view) {
  NOTIMPLEMENTED();
  // TODO(ivankr): there has to be an Init() call, otherwise |window_|
  // is left uninitialized and will eventually crash.
  Init();
}

RenderWidgetHost* RenderWidgetHostViewAura::GetRenderWidgetHost() const {
  return host_;
}

void RenderWidgetHostViewAura::DidBecomeSelected() {
  host_->WasRestored();
}

void RenderWidgetHostViewAura::WasHidden() {
  host_->WasHidden();
}

void RenderWidgetHostViewAura::SetSize(const gfx::Size& size) {
  SetBounds(gfx::Rect(window_->bounds().origin(), size));
}

void RenderWidgetHostViewAura::SetBounds(const gfx::Rect& rect) {
  window_->SetBounds(rect);
  host_->WasResized();
}

gfx::NativeView RenderWidgetHostViewAura::GetNativeView() const {
  return window_;
}

gfx::NativeViewId RenderWidgetHostViewAura::GetNativeViewId() const {
  return static_cast<gfx::NativeViewId>(NULL);
}

void RenderWidgetHostViewAura::MovePluginWindows(
    const std::vector<webkit::npapi::WebPluginGeometry>& moves) {
  // We don't support windowed plugins.
}

void RenderWidgetHostViewAura::Focus() {
  window_->Focus();
}

void RenderWidgetHostViewAura::Blur() {
  window_->Blur();
}

bool RenderWidgetHostViewAura::HasFocus() const {
  return window_->HasFocus();
}

void RenderWidgetHostViewAura::Show() {
  window_->Show();
}

void RenderWidgetHostViewAura::Hide() {
  window_->Hide();
}

bool RenderWidgetHostViewAura::IsShowing() {
  return window_->IsVisible();
}

gfx::Rect RenderWidgetHostViewAura::GetViewBounds() const {
  return window_->bounds();
}

void RenderWidgetHostViewAura::UpdateCursor(const WebCursor& cursor) {
  current_cursor_ = cursor;
  UpdateCursorIfOverSelf();
}

void RenderWidgetHostViewAura::SetIsLoading(bool is_loading) {
  is_loading_ = is_loading;
  UpdateCursorIfOverSelf();
}

void RenderWidgetHostViewAura::TextInputStateChanged(
    ui::TextInputType type,
    bool can_compose_inline) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAura::ImeCancelComposition() {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAura::DidUpdateBackingStore(
    const gfx::Rect& scroll_rect, int scroll_dx, int scroll_dy,
    const std::vector<gfx::Rect>& copy_rects) {
  if (!window_->IsVisible())
    return;

  if (!scroll_rect.IsEmpty())
    window_->SchedulePaintInRect(scroll_rect);

  for (size_t i = 0; i < copy_rects.size(); ++i) {
    gfx::Rect rect = copy_rects[i].Subtract(scroll_rect);
    if (rect.IsEmpty())
      continue;

    window_->SchedulePaintInRect(rect);
  }
}

void RenderWidgetHostViewAura::RenderViewGone(base::TerminationStatus status,
                                              int error_code) {
  UpdateCursorIfOverSelf();
  Destroy();
}

void RenderWidgetHostViewAura::Destroy() {
  delete window_;
}

void RenderWidgetHostViewAura::SetTooltipText(const string16& tooltip_text) {
  tooltip_ = tooltip_text;
}

BackingStore* RenderWidgetHostViewAura::AllocBackingStore(
    const gfx::Size& size) {
  return new BackingStoreSkia(host_, size);
}

#if defined(UI_COMPOSITOR_IMAGE_TRANSPORT)
void RenderWidgetHostViewAura::AcceleratedSurfaceNew(
      int32 width,
      int32 height,
      uint64* surface_id,
      TransportDIB::Handle* surface_handle) {
  scoped_refptr<AcceleratedSurfaceContainerLinux> surface(
      AcceleratedSurfaceContainerLinux::Create(gfx::Size(width, height)));
  if (!surface->Initialize(surface_id)) {
    LOG(ERROR) << "Failed to create AcceleratedSurfaceContainer";
    return;
  }
  *surface_handle = surface->Handle();

  accelerated_surface_containers_[*surface_id] = surface;
}

void RenderWidgetHostViewAura::AcceleratedSurfaceBuffersSwapped(
      uint64 surface_id,
      int32 route_id,
      int gpu_host_id) {
  window_->layer()->SetExternalTexture(
      accelerated_surface_containers_[surface_id]->GetTexture());
  glFlush();

  if (!window_->layer()->GetCompositor()) {
    // We have no compositor, so we have no way to display the surface
    AcknowledgeSwapBuffers(route_id, gpu_host_id);  // Must still send the ACK
  } else {
    window_->layer()->ScheduleDraw();

    // Add sending an ACK to the list of things to do OnCompositingEnded
    on_compositing_ended_callbacks_.push_back(
        base::Bind(AcknowledgeSwapBuffers, route_id, gpu_host_id));
    ui::Compositor* compositor = window_->layer()->GetCompositor();
    if (!compositor->HasObserver(this))
      compositor->AddObserver(this);
  }
}

void RenderWidgetHostViewAura::AcceleratedSurfaceRelease(uint64 surface_id) {
  accelerated_surface_containers_.erase(surface_id);
}
#endif

void RenderWidgetHostViewAura::SetBackground(const SkBitmap& background) {
  RenderWidgetHostView::SetBackground(background);
  host_->SetBackground(background);
#if defined(UI_COMPOSITOR_IMAGE_TRANSPORT)
  window_->layer()->SetFillsBoundsOpaquely(background.isOpaque());
#endif
}

#if defined(OS_POSIX)
void RenderWidgetHostViewAura::GetDefaultScreenInfo(
    WebKit::WebScreenInfo* results) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAura::GetScreenInfo(WebKit::WebScreenInfo* results) {
  NOTIMPLEMENTED();
}

gfx::Rect RenderWidgetHostViewAura::GetRootWindowBounds() {
  NOTIMPLEMENTED();
  return gfx::Rect();
}
#endif

void RenderWidgetHostViewAura::SetVisuallyDeemphasized(const SkColor* color,
                                                       bool animate) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAura::UnhandledWheelEvent(
    const WebKit::WebMouseWheelEvent& event) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAura::SetHasHorizontalScrollbar(
    bool has_horizontal_scrollbar) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAura::SetScrollOffsetPinning(
    bool is_pinned_to_left, bool is_pinned_to_right) {
  NOTIMPLEMENTED();
}

#if defined(OS_WIN)
void RenderWidgetHostViewAura::WillWmDestroy() {
  // Nothing to do.
}

void RenderWidgetHostViewAura::ShowCompositorHostWindow(bool show) {
  // Nothing to do.
}
#endif

#if defined(UI_COMPOSITOR_IMAGE_TRANSPORT)
gfx::PluginWindowHandle RenderWidgetHostViewAura::GetCompositingSurface() {
  // The GPU process renders to an offscreen surface (created by the GPU
  // process), which is later displayed by the browser. As the GPU process
  // creates this surface, we can return any non-zero value.
  return 1;
}
#else
gfx::PluginWindowHandle RenderWidgetHostViewAura::GetCompositingSurface() {
  // TODO(oshima): The original implementation was broken as
  // GtkNativeViewManager doesn't know about NativeWidgetGtk. Figure
  // out if this makes sense without compositor. If it does, then find
  // out the right way to handle.
  NOTIMPLEMENTED();
  return gfx::kNullPluginWindow;
}
#endif

bool RenderWidgetHostViewAura::LockMouse() {
  NOTIMPLEMENTED();
  return false;
}

void RenderWidgetHostViewAura::UnlockMouse() {
  NOTIMPLEMENTED();
  host_->LostMouseLock();
}

////////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostViewAura, aura::WindowDelegate implementation:

void RenderWidgetHostViewAura::OnBoundsChanged(const gfx::Rect& old_bounds,
                                               const gfx::Rect& new_bounds) {
  // We don't care about this one, we are always sized via SetSize() or
  // SetBounds().
}

void RenderWidgetHostViewAura::OnFocus() {
  host_->GotFocus();
}

void RenderWidgetHostViewAura::OnBlur() {
  host_->Blur();
}

bool RenderWidgetHostViewAura::OnKeyEvent(aura::KeyEvent* event) {
  NativeWebKeyboardEvent webkit_event(event);
  host_->ForwardKeyboardEvent(webkit_event);
  return true;
}

gfx::NativeCursor RenderWidgetHostViewAura::GetCursor(const gfx::Point& point) {
  // TODO(beng): talk to beng before implementing this.
  //NOTIMPLEMENTED();
  return gfx::kNullCursor;
}

int RenderWidgetHostViewAura::GetNonClientComponent(
    const gfx::Point& point) const {
  return HTCLIENT;
}

bool RenderWidgetHostViewAura::OnMouseEvent(aura::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSEWHEEL)
    host_->ForwardWheelEvent(content::MakeWebMouseWheelEvent(event));
  else
    host_->ForwardMouseEvent(content::MakeWebMouseEvent(event));

  // Return true so that we receive released/drag events.
  return true;
}

ui::TouchStatus RenderWidgetHostViewAura::OnTouchEvent(
    aura::TouchEvent* event) {
  // Update the touch event first.
  WebKit::WebTouchPoint* point = content::UpdateWebTouchEvent(event,
      &touch_event_);

  // Forward the touch event only if a touch point was updated.
  if (point) {
    host_->ForwardTouchEvent(touch_event_);
    UpdateWebTouchEventAfterDispatch(&touch_event_, point);
  }

  return DecideTouchStatus(touch_event_, point);
}

bool RenderWidgetHostViewAura::ShouldActivate(aura::Event* event) {
  return false;
}

void RenderWidgetHostViewAura::OnActivated() {
}

void RenderWidgetHostViewAura::OnLostActive() {
}

void RenderWidgetHostViewAura::OnCaptureLost() {
  host_->LostCapture();
}

void RenderWidgetHostViewAura::OnPaint(gfx::Canvas* canvas) {
  if (!window_->IsVisible())
    return;
  BackingStore* backing_store = host_->GetBackingStore(true);
  if (backing_store) {
    static_cast<BackingStoreSkia*>(backing_store)->SkiaShowRect(gfx::Point(),
                                                                canvas);
  } else {
    canvas->FillRect(SK_ColorWHITE,
                     gfx::Rect(gfx::Point(), window_->bounds().size()));
  }
}

void RenderWidgetHostViewAura::OnWindowDestroying() {
}

void RenderWidgetHostViewAura::OnWindowDestroyed() {
  host_->ViewDestroyed();
  delete this;
}

void RenderWidgetHostViewAura::OnWindowVisibilityChanged(bool visible) {
}

#if defined(UI_COMPOSITOR_IMAGE_TRANSPORT)
////////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostViewAura, ui::CompositorDelegate implementation:

void RenderWidgetHostViewAura::OnCompositingEnded(ui::Compositor* compositor) {
  for (std::vector< base::Callback<void(void)> >::const_iterator
      it = on_compositing_ended_callbacks_.begin();
      it != on_compositing_ended_callbacks_.end(); ++it) {
    it->Run();
  }
  on_compositing_ended_callbacks_.clear();
  compositor->RemoveObserver(this);
}
#endif

////////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostViewAura, private:

void RenderWidgetHostViewAura::UpdateCursorIfOverSelf() {
  //NOTIMPLEMENTED();
  // TODO(beng): See RenderWidgetHostViewWin.
}
