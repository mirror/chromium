// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_display_unified.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "services/ui/display/screen_manager.h"
#include "services/ui/public/interfaces/cursor/cursor_struct_traits.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/threaded_image_cursors.h"
#include "ui/display/display.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/platform_window/platform_ime_controller.h"
#include "ui/platform_window/platform_window.h"

#if defined(USE_X11)
#include "ui/platform_window/x11/x11_window.h"
#elif defined(USE_OZONE)
#include "ui/events/ozone/chromeos/cursor_controller.h"
#include "ui/ozone/public/cursor_factory_ozone.h"
#include "ui/ozone/public/ozone_platform.h"
#endif

namespace ui {
namespace ws {

PlatformDisplayUnified::PlatformDisplayUnified(
    ServerWindow* root_window,
    const display::ViewportMetrics& metrics,
    std::unique_ptr<ThreadedImageCursors> image_cursors)
    : root_window_(root_window),
      image_cursors_(std::move(image_cursors)),
      metrics_(metrics),
      widget_(gfx::kNullAcceleratedWidget) {}

PlatformDisplayUnified::~PlatformDisplayUnified() {
  ui::CursorController::GetInstance()->ClearCursorConfigForWindow(
      GetAcceleratedWidget());

  // Don't notify the delegate from the destructor.
  delegate_ = nullptr;

  frame_generator_.reset();
  image_cursors_.reset();
  // Destroy the PlatformWindows early on as they may call us back during
  // destruction and we want to be in a known state. But destroy the surface(s) 
  // and ThreadedImageCursors first because they can still be using the platform
  // windows.
  platform_window_.reset();
  platform_windows_.clear();
}

void PlatformDisplayUnified::SetMirrors(
    const std::vector<display::Display>& mirrors) {
  LOG(ERROR) << "MSW PlatformDisplayUnified::SetMirrors "; 
  // TODO(msw): Any way to keep and re-use existing windows? Pass from manager? 
  platform_windows_.clear();
  // Check that the dimensions are reasonable.
  for (const display::Display& mirror : mirrors) {
    // TODO(msw): pass per-mirror metrics or adjust position/scale? 
    // const gfx::Rect& bounds = metrics_.bounds_in_pixels;
    const gfx::Rect& bounds = mirror.bounds();
    DCHECK(!bounds.size().IsEmpty());
    // TODO(msw): Extract a PlatformWindow::Create function from pddefault? 
    // TODO(msw): Create a PlatformWindow for each physical display.
// #if defined(USE_X11)
//     platform_windows_.push_back(std::make_unique<ui::X11Window>(this, bounds));
// #elif defined(USE_OZONE)
//     platform_windows_.push_back(
//         delegate_->GetOzonePlatform()->CreatePlatformWindow(this, bounds));;
// #else
//     NOTREACHED() << "Unsupported platform";
// #endif
  }
}

EventSink* PlatformDisplayUnified::GetEventSink() {
  return delegate_->GetEventSink();
}

void PlatformDisplayUnified::Init(PlatformDisplayDelegate* delegate) {
  LOG(ERROR) << "MSW PlatformDisplayUnified::Init "; 

  DCHECK(delegate);
  delegate_ = delegate;

  const gfx::Rect& bounds = metrics_.bounds_in_pixels;
  DCHECK(!bounds.size().IsEmpty());

  // TODO(msw): Plumb in mirrors earlier? Maybe call up to static_cast<Display>(delegate)->display_manager()? 
  // TODO(msw): Create PlatformWindowUnified or similar? 
#if defined(USE_X11)
  platform_window_ = base::MakeUnique<ui::X11Window>(this, bounds);
#elif defined(USE_OZONE)
  platform_window_ =
      delegate_->GetOzonePlatform()->CreatePlatformWindow(this, bounds);
#else
  NOTREACHED() << "Unsupported platform";
#endif

  platform_window_->Show();
  if (image_cursors_) {
    image_cursors_->SetDisplay(delegate_->GetDisplay(),
                               metrics_.device_scale_factor);
  }
}

void PlatformDisplayUnified::SetViewportSize(const gfx::Size& size) {
  platform_window_->SetBounds(gfx::Rect(size));
}

void PlatformDisplayUnified::SetTitle(const base::string16& title) {
  platform_window_->SetTitle(title);
}

void PlatformDisplayUnified::SetCapture() {
  platform_window_->SetCapture();
}

void PlatformDisplayUnified::ReleaseCapture() {
  platform_window_->ReleaseCapture();
}

void PlatformDisplayUnified::SetCursor(const ui::CursorData& cursor_data) {
  if (!image_cursors_)
    return;

  ui::CursorType cursor_type = cursor_data.cursor_type();

#if defined(USE_OZONE)
  if (cursor_type != ui::CursorType::kCustom) {
    // |platform_window_| is destroyed after |image_cursors_|, so it is
    // guaranteed to outlive |image_cursors_|.
    image_cursors_->SetCursor(cursor_type, platform_window_.get());
  } else {
    ui::Cursor native_cursor(cursor_type);
    // In Ozone builds, we have an interface available which turns bitmap data
    // into platform cursors.
    ui::CursorFactoryOzone* cursor_factory =
        delegate_->GetOzonePlatform()->GetCursorFactoryOzone();
    native_cursor.SetPlatformCursor(cursor_factory->CreateAnimatedCursor(
        cursor_data.cursor_frames(), cursor_data.hotspot_in_pixels(),
        cursor_data.frame_delay().InMilliseconds(),
        cursor_data.scale_factor()));
    platform_window_->SetCursor(native_cursor.platform());
  }
#else
  // Outside of ozone builds, there isn't a single interface for creating
  // PlatformCursors. The closest thing to one is in //content/ instead of
  // //ui/ which means we can't use it from here. For now, just don't handle
  // custom image cursors.
  //
  // TODO(erg): Once blink speaks directly to mus, make blink perform its own
  // cursor management on its own mus windows so we can remove Webcursor from
  // //content/ and do this in way that's safe cross-platform, instead of as an
  // ozone-specific hack.
  if (cursor_type == ui::CursorType::kCustom) {
    NOTIMPLEMENTED() << "No custom cursor support on non-ozone yet.";
    cursor_type = ui::CursorType::kPointer;
  }
  image_cursors_->SetCursor(cursor_type, platform_window_.get());
#endif
}

void PlatformDisplayUnified::MoveCursorTo(
    const gfx::Point& window_pixel_location) {
  platform_window_->MoveCursorTo(window_pixel_location);
}

void PlatformDisplayUnified::SetCursorSize(const ui::CursorSize& cursor_size) {
  image_cursors_->SetCursorSize(cursor_size);
}

void PlatformDisplayUnified::ConfineCursorToBounds(
    const gfx::Rect& pixel_bounds) {
  confine_cursor_bounds_ = pixel_bounds;
  platform_window_->ConfineCursorToBounds(pixel_bounds);
}

void PlatformDisplayUnified::UpdateTextInputState(
    const ui::TextInputState& state) {
  ui::PlatformImeController* ime = platform_window_->GetPlatformImeController();
  if (ime)
    ime->UpdateTextInputState(state);
}

void PlatformDisplayUnified::SetImeVisibility(bool visible) {
  ui::PlatformImeController* ime = platform_window_->GetPlatformImeController();
  if (ime)
    ime->SetImeVisibility(visible);
}

FrameGenerator* PlatformDisplayUnified::GetFrameGenerator() {
  return frame_generator_.get();
}

void PlatformDisplayUnified::UpdateViewportMetrics(
    const display::ViewportMetrics& metrics) {
  if (metrics_ == metrics)
    return;

  gfx::Rect bounds = platform_window_->GetBounds();
  if (bounds.size() != metrics.bounds_in_pixels.size()) {
    bounds.set_size(metrics.bounds_in_pixels.size());
    platform_window_->SetBounds(bounds);
  }

  metrics_ = metrics;
  if (frame_generator_) {
    frame_generator_->SetDeviceScaleFactor(metrics_.device_scale_factor);
    frame_generator_->OnWindowSizeChanged(metrics_.bounds_in_pixels.size());
  }
}

const display::ViewportMetrics& PlatformDisplayUnified::GetViewportMetrics() {
  return metrics_;
}

gfx::AcceleratedWidget PlatformDisplayUnified::GetAcceleratedWidget() const {
  return widget_;
}

void PlatformDisplayUnified::SetCursorConfig(
    display::Display::Rotation rotation,
    float scale) {
  ui::CursorController::GetInstance()->SetCursorConfigForWindow(
      GetAcceleratedWidget(), rotation, scale);
}

void PlatformDisplayUnified::OnBoundsChanged(const gfx::Rect& new_bounds) {
  // Redo cursor confinement for new bounds, in case the window origin moved.
  if (!confine_cursor_bounds_.IsEmpty())
    platform_window_->ConfineCursorToBounds(confine_cursor_bounds_);
}

void PlatformDisplayUnified::OnDamageRect(const gfx::Rect& damaged_region) {
  if (frame_generator_)
    frame_generator_->OnWindowDamaged();
}

void PlatformDisplayUnified::DispatchEvent(ui::Event* event) {
  // TODO(msw): Convert coords btw virtual/unified & screen/display as needed? 
  // Event location and event root location are the same, and both in pixels
  // and display coordinates.
  if (event->IsScrollEvent()) {
    // TODO(moshayedi): crbug.com/602859. Dispatch scroll events as
    // they are once we have proper support for scroll events.

    ui::PointerEvent pointer_event(
        ui::MouseWheelEvent(*event->AsScrollEvent()));
    SendEventToSink(&pointer_event);
  } else if (event->IsMouseEvent()) {
    ui::PointerEvent pointer_event(*event->AsMouseEvent());
    SendEventToSink(&pointer_event);
  } else if (event->IsTouchEvent()) {
    ui::PointerEvent pointer_event(*event->AsTouchEvent());
    SendEventToSink(&pointer_event);
  } else {
    SendEventToSink(event);
  }
}

void PlatformDisplayUnified::OnCloseRequest() {
  // TODO(tonikitoo): Handle a close request in external window mode. The window
  // should be closed by the WS and it shouldn't involve ScreenManager.
  const int64_t display_id = delegate_->GetDisplay().id();
  display::ScreenManager::GetInstance()->RequestCloseDisplay(display_id);
}

void PlatformDisplayUnified::OnClosed() {}

void PlatformDisplayUnified::OnWindowStateChanged(
    ui::PlatformWindowState new_state) {}

void PlatformDisplayUnified::OnLostCapture() {
  delegate_->OnNativeCaptureLost();
}

void PlatformDisplayUnified::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget,
    float device_scale_factor) {
  LOG(ERROR) << "MSW PlatformDisplayUnified::OnAcceleratedWidgetAvailable A " << this << " " << widget << " " << device_scale_factor; 
  // TODO(msw): Scott suggests setting widget_ to std::numeric_limits<gfx::AcceleratedWidget>::max() - 1; ... 
  // This will get called after Init() is called, either synchronously as part
  // of the Init() callstack or async after Init() has returned, depending on
  // the platform.
  DCHECK_EQ(gfx::kNullAcceleratedWidget, widget_);
  widget_ = widget;
  delegate_->OnAcceleratedWidgetAvailable();

  viz::mojom::CompositorFrameSinkAssociatedPtr compositor_frame_sink;
  viz::mojom::DisplayPrivateAssociatedPtr display_private;
  viz::mojom::CompositorFrameSinkClientPtr compositor_frame_sink_client;
  viz::mojom::CompositorFrameSinkClientRequest
      compositor_frame_sink_client_request =
          mojo::MakeRequest(&compositor_frame_sink_client);

  root_window_->CreateRootCompositorFrameSink(
      widget_, mojo::MakeRequest(&compositor_frame_sink),
      std::move(compositor_frame_sink_client),
      mojo::MakeRequest(&display_private));

  display_private->SetDisplayVisible(true);
  frame_generator_ = base::MakeUnique<FrameGenerator>();
  auto frame_sink_client_binding =
      base::MakeUnique<CompositorFrameSinkClientBinding>(
          frame_generator_.get(),
          std::move(compositor_frame_sink_client_request),
          std::move(compositor_frame_sink), std::move(display_private));
  frame_generator_->Bind(std::move(frame_sink_client_binding));
  frame_generator_->OnWindowSizeChanged(root_window_->bounds().size());
  frame_generator_->SetDeviceScaleFactor(metrics_.device_scale_factor);
}

void PlatformDisplayUnified::OnAcceleratedWidgetDestroyed() {
  NOTREACHED();
}

void PlatformDisplayUnified::OnActivationChanged(bool active) {}

}  // namespace ws
}  // namespace ui
