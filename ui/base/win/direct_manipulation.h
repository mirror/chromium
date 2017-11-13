// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_WIN_DIRECT_MANIPULATION_H_
#define UI_BASE_WIN_DIRECT_MANIPULATION_H_

#include <windows.h>

#include <directmanipulation.h>
#include <wrl.h>
#include <memory>

#include "base/macros.h"
#include "ui/base/ui_base_export.h"
#include "ui/base/win/window_event_target.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {

class DirectManipulationHelper;

class DirectManipulationHandler
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<
              Microsoft::WRL::RuntimeClassType::ClassicCom>,
          Microsoft::WRL::Implements<
              Microsoft::WRL::RuntimeClassFlags<
                  Microsoft::WRL::RuntimeClassType::ClassicCom>,
              Microsoft::WRL::FtmBase,
              IDirectManipulationViewportEventHandler>> {
 public:
  HRESULT STDMETHODCALLTYPE
  OnViewportStatusChanged(_In_ IDirectManipulationViewport* viewport,
                          _In_ DIRECTMANIPULATION_STATUS current,
                          _In_ DIRECTMANIPULATION_STATUS previous) override;

  HRESULT STDMETHODCALLTYPE
  OnViewportUpdated(_In_ IDirectManipulationViewport* viewport) override;

  HRESULT STDMETHODCALLTYPE
  OnContentUpdated(_In_ IDirectManipulationViewport* viewport,
                   _In_ IDirectManipulationContent* content) override;
  void SetDirectManipulationHelper(DirectManipulationHelper* helper);
  void SetWindowEventTarget(WindowEventTarget* event_target);

 private:
  DirectManipulationHelper* helper_;
  WindowEventTarget* event_target_;
  float last_scale_;
  float last_x_offset_;
  float last_y_offset_;
};

// Windows 10 provides a new API called Direct Manipulation which generates
// smooth scroll events via WM_MOUSEWHEEL messages with predictable deltas
// on high precision touch pads. This basically requires the application window
// to register as a Direct Manipulation consumer. The way mouse wheel messages
// are dispatched is
// 1. The foreground window is checked to see if it is a Direct Manipulation
//    consumer.
// 2. If it is then Direct Manipulation takes over and sends the following
//    messages. WM_POINTERACTIVATE, WM_POINTERDOWN and DM_POINTERHITTEST.
// 3. It then posts WM_MOUSEWHEEL messages with precision deltas which vary
//    based on the amount of the scroll.
// 4. If the foreground window is not a Direct Manipulation consumer, it
//    then takes a fallback route where it posts WM_MOUSEWHEEL messages
//    with precision but varying deltas to the window. There is a also
//    a slight delay in receiving the first set of mouse wheel messages.
//    This causes scrolling to appear janky and jumpy.
// Our approach for addressing this is to do the absolute minimum to
// register our window as a Direct Manipulation consumer. This class
// provides the necessary functionality to register the passed in HWND as a
// Direct Manipulation consumer. We don't rely on Direct manipulation
// to do the smooth scrolling in the background thread as documented on
// msdn.
class UI_BASE_EXPORT DirectManipulationHelper {
 public:
  // Creates an instance of this class if Direct Manipulation is enabled on
  // the platform. If not returns NULL.
  static std::unique_ptr<DirectManipulationHelper> CreateInstance();

  // This function instantiates Direct Manipulation and creates a viewport for
  // the passed in |window|.
  // consumer. Most of the code is boiler plate and is based on the sample.
  void Initialize(HWND window, WindowEventTarget* event_target);

  // Sets the bounds of the fake Direct manipulation viewport to match those
  // of the legacy window.
  void SetBounds(const gfx::Rect& bounds);

  // Registers and activates the passed in |window| as a Direct Manipulation
  // consumer.
  void Activate(HWND window);

  // Deactivates Direct Manipulation processing on the passed in |window|.
  void Deactivate(HWND window);

  void ResetViewport();

  void OnPointerHitTest(WPARAM w_param);

  void OnTimer(WPARAM w_param);

  ~DirectManipulationHelper();

 private:
  DirectManipulationHelper();

  static DirectManipulationHelper instance_;

  Microsoft::WRL::ComPtr<IDirectManipulationManager2> manager_;
  Microsoft::WRL::ComPtr<IDirectManipulationUpdateManager> update_manager_;
  Microsoft::WRL::ComPtr<IDirectManipulationViewport2> view_port_outer_;
  Microsoft::WRL::ComPtr<DirectManipulationHandler> event_handler_;
  UINT_PTR render_timer_;
  DWORD view_port_handler_cookie_;

  RECT const default_rect_ = {0, 0, 100, 100};

  DISALLOW_COPY_AND_ASSIGN(DirectManipulationHelper);
};

}  // namespace ui

#endif  // UI_BASE_WIN_DIRECT_MANIPULATION_H_
