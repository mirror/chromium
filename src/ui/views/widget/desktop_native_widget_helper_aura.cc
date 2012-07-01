// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_native_widget_helper_aura.h"

#include "ui/aura/client/dispatcher_client.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/cursor_manager.h"
#include "ui/aura/desktop/desktop_activation_client.h"
#include "ui/aura/desktop/desktop_dispatcher_client.h"
#include "ui/aura/focus_manager.h"
#include "ui/aura/root_window.h"
#include "ui/aura/shared/compound_event_filter.h"
#include "ui/aura/shared/input_method_event_filter.h"
#include "ui/aura/shared/root_window_capture_client.h"
#include "ui/aura/window_property.h"
#include "ui/views/widget/native_widget_aura.h"

#if defined(OS_WIN)
#include "ui/base/win/hwnd_subclass.h"
#include "ui/views/widget/widget_message_filter.h"
#elif defined(USE_X11)
#include "ui/views/widget/x11_desktop_handler.h"
#include "ui/views/widget/x11_window_event_filter.h"
#endif

DECLARE_WINDOW_PROPERTY_TYPE(aura::Window*);

namespace views {

DEFINE_WINDOW_PROPERTY_KEY(
    aura::Window*, kViewsWindowForRootWindow, NULL);

namespace {

// Client that always offsets the passed in point by the RootHost's origin.
class RootWindowScreenPositionClient
    : public aura::client::ScreenPositionClient {
 public:
  explicit RootWindowScreenPositionClient(aura::RootWindow* root_window)
      : root_window_(root_window) {}
  virtual ~RootWindowScreenPositionClient() {}

  // aura::client::ScreenPositionClient:
  virtual void ConvertToScreenPoint(gfx::Point* screen_point) OVERRIDE {
    gfx::Point origin = root_window_->GetHostOrigin();
    screen_point->Offset(origin.x(), origin.y());
  }

 private:
  aura::RootWindow* root_window_;
};

// Client that always offsets by the toplevel RootWindow of the passed in child
// NativeWidgetAura.
class EmbeddedWindowScreenPositionClient
    : public aura::client::ScreenPositionClient {
 public:
  explicit EmbeddedWindowScreenPositionClient(NativeWidgetAura* widget)
      : widget_(widget) {}
  virtual ~EmbeddedWindowScreenPositionClient() {}

  // aura::client::ScreenPositionClient:
  virtual void ConvertToScreenPoint(gfx::Point* screen_point) OVERRIDE {
    aura::RootWindow* root =
        widget_->GetNativeWindow()->GetRootWindow()->GetRootWindow();
    gfx::Point origin = root->GetHostOrigin();
    screen_point->Offset(origin.x(), origin.y());
  }

 private:
  NativeWidgetAura* widget_;
};

}  // namespace

DesktopNativeWidgetHelperAura::DesktopNativeWidgetHelperAura(
    NativeWidgetAura* widget)
    : widget_(widget),
      root_window_event_filter_(NULL),
      is_embedded_window_(false) {
}

DesktopNativeWidgetHelperAura::~DesktopNativeWidgetHelperAura() {
  if (root_window_event_filter_) {
#if defined(USE_X11)
    root_window_event_filter_->RemoveFilter(x11_window_event_filter_.get());
#endif

    root_window_event_filter_->RemoveFilter(input_method_filter_.get());
  }
}

// static
aura::Window* DesktopNativeWidgetHelperAura::GetViewsWindowForRootWindow(
    aura::RootWindow* root) {
  return root ? root->GetProperty(kViewsWindowForRootWindow) : NULL;
}

void DesktopNativeWidgetHelperAura::PreInitialize(
    aura::Window* window,
    const Widget::InitParams& params) {
#if !defined(OS_WIN)
  // We don't want the status bubble or the omnibox to get their own root
  // window on the desktop; on Linux
  //
  // TODO(erg): This doesn't map perfectly to what I want to do. TYPE_POPUP is
  // used for lots of stuff, like dragged tabs, and I only want this to trigger
  // for the status bubble and the omnibox.
  if (params.type == Widget::InitParams::TYPE_POPUP ||
      params.type == Widget::InitParams::TYPE_BUBBLE) {
    is_embedded_window_ = true;
    position_client_.reset(new EmbeddedWindowScreenPositionClient(widget_));
    aura::client::SetScreenPositionClient(window, position_client_.get());
    return;
  } else if (params.type == Widget::InitParams::TYPE_CONTROL) {
    return;
  }
#endif

  gfx::Rect bounds = params.bounds;
  if (bounds.IsEmpty()) {
    // We must pass some non-zero value when we initialize a RootWindow. This
    // will probably be SetBounds()ed soon.
    bounds.set_size(gfx::Size(100, 100));
  }
  // TODO(erg): Implement aura::CursorManager::Delegate to control
  // cursor's shape and visibility.

  aura::FocusManager* focus_manager = NULL;
  aura::DesktopActivationClient* activation_client = NULL;
#if defined(USE_X11)
  focus_manager = X11DesktopHandler::get()->get_focus_manager();
  activation_client = X11DesktopHandler::get()->get_activation_client();
#else
  // TODO(ben): This is almost certainly wrong; I suspect that the windows
  // build will need a singleton like above.
  focus_manager = new aura::FocusManager;
  activation_client = new aura::DesktopActivationClient(focus_manager);
#endif

  root_window_.reset(new aura::RootWindow(bounds));
  root_window_->SetProperty(kViewsWindowForRootWindow, window);
  root_window_->Init();
  root_window_->set_focus_manager(focus_manager);

  // No event filter for aura::Env. Create CompoundEvnetFilter per RootWindow.
  root_window_event_filter_ = new aura::shared::CompoundEventFilter;
  // Pass ownership of the filter to the root_window.
  root_window_->SetEventFilter(root_window_event_filter_);

  input_method_filter_.reset(new aura::shared::InputMethodEventFilter());
  input_method_filter_->SetInputMethodPropertyInRootWindow(root_window_.get());
  root_window_event_filter_->AddFilter(input_method_filter_.get());

  capture_client_.reset(
      new aura::shared::RootWindowCaptureClient(root_window_.get()));

#if defined(USE_X11)
  x11_window_event_filter_.reset(
      new X11WindowEventFilter(root_window_.get(), activation_client, widget_));
  x11_window_event_filter_->SetUseHostWindowBorders(false);
  root_window_event_filter_->AddFilter(x11_window_event_filter_.get());
#endif

  root_window_->AddRootWindowObserver(this);

  aura::client::SetActivationClient(root_window_.get(), activation_client);
  aura::client::SetDispatcherClient(root_window_.get(),
                                    new aura::DesktopDispatcherClient);

  position_client_.reset(
      new RootWindowScreenPositionClient(root_window_.get()));
  aura::client::SetScreenPositionClient(window, position_client_.get());
}

void DesktopNativeWidgetHelperAura::PostInitialize() {
#if defined(OS_WIN)
  DCHECK(root_window_->GetAcceleratedWidget());
  hwnd_message_filter_.reset(new WidgetMessageFilter(root_window_.get(),
      widget_->GetWidget()));
  ui::HWNDSubclass::AddFilterToTarget(root_window_->GetAcceleratedWidget(),
                                      hwnd_message_filter_.get());
#endif
}

void DesktopNativeWidgetHelperAura::ShowRootWindow() {
  if (root_window_.get())
    root_window_->ShowRootWindow();
}

aura::RootWindow* DesktopNativeWidgetHelperAura::GetRootWindow() {
  return root_window_.get();
}

gfx::Rect DesktopNativeWidgetHelperAura::ModifyAndSetBounds(
    const gfx::Rect& bounds) {
  gfx::Rect out_bounds = bounds;
  if (root_window_.get() && !out_bounds.IsEmpty()) {
    root_window_->SetHostBounds(out_bounds);
    out_bounds.set_x(0);
    out_bounds.set_y(0);
  } else if (is_embedded_window_) {
    // The caller expects windows we consider "embedded" to be placed in the
    // screen coordinate system. So we need to offset the root window's
    // position (which is in screen coordinates) from these bounds.
    aura::RootWindow* root =
        widget_->GetNativeWindow()->GetRootWindow()->GetRootWindow();
    gfx::Point point = root->GetHostOrigin();
    out_bounds.set_x(out_bounds.x() - point.x());
    out_bounds.set_y(out_bounds.y() - point.y());
  }

  return out_bounds;
}

////////////////////////////////////////////////////////////////////////////////
// DesktopNativeWidgetHelperAura, aura::RootWindowObserver implementation:

void DesktopNativeWidgetHelperAura::OnRootWindowResized(
    const aura::RootWindow* root,
    const gfx::Size& old_size) {
  DCHECK_EQ(root, root_window_.get());
  widget_->SetBounds(gfx::Rect(root->GetHostOrigin(),
                               root->GetHostSize()));
}

void DesktopNativeWidgetHelperAura::OnRootWindowHostClosed(
    const aura::RootWindow* root) {
  DCHECK_EQ(root, root_window_.get());
  widget_->GetWidget()->Close();
}

}  // namespace views
