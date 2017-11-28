// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_WIN_DIRECT_MANIPULATION_H_
#define UI_BASE_WIN_DIRECT_MANIPULATION_H_

#include <windows.h>

#include <directmanipulation.h>
#include <wrl.h>
#include <memory>

#include "base/cancelable_callback.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/ui_base_export.h"
#include "ui/base/win/window_event_target.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {

UI_BASE_EXPORT extern const base::Feature kPrecisionTouchpad;

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
  void Initialize(DirectManipulationHelper* helper,
                  WindowEventTarget* event_target);

 private:
  DirectManipulationHelper* helper_;
  WindowEventTarget* event_target_;
  bool pinch_begin_sent_;
  float last_scale_;
  float last_x_offset_;
  float last_y_offset_;
};

// Windows 10 provides a new API called Direct Manipulation which generates
// smooth scroll and scale factor via IDirectManipulationViewportEventHandler
// on precision touchpad.
// 1. The foreground window is checked to see if it is a Direct Manipulation
//    consumer.
// 2. Call SetContact in Direct Manipulation takes over the following scrolling
//    when DM_POINTERHITTEST.
// 3. OnViewportStatusChanged will be called when the gesture phase change.
//    OnContentUpdated will be called when the gesture update.
class UI_BASE_EXPORT DirectManipulationHelper {
 public:
  // Creates an instance of this class if Direct Manipulation is enabled on
  // the platform. If not returns nullptr.
  static std::unique_ptr<DirectManipulationHelper> CreateInstance();

  // This function instantiates Direct Manipulation and creates a viewport for
  // the passed in |window|.
  // consumer. Most of the code is boiler plate and is based on the sample.
  void Initialize(HWND window, WindowEventTarget* event_target);

  // Registers and activates the passed in |window| as a Direct Manipulation
  // consumer.
  void Activate(HWND window);

  // Deactivates Direct Manipulation processing on the passed in |window|.
  void Deactivate(HWND window);

  // Reset the fake viewport for gesture end.
  void ResetViewport();

  // Pass the pointer hit test to Direct Manipulation.
  void OnPointerHitTest(WPARAM w_param);

  ~DirectManipulationHelper();

 private:
  DirectManipulationHelper();

  void Update();

  void StartTimer();

  void StopTimer();

  RECT const default_rect_ = {0, 0, 1000, 1000};

  Microsoft::WRL::ComPtr<IDirectManipulationManager2> manager_;
  Microsoft::WRL::ComPtr<IDirectManipulationUpdateManager> update_manager_;
  Microsoft::WRL::ComPtr<IDirectManipulationViewport2> view_port_outer_;
  Microsoft::WRL::ComPtr<DirectManipulationHandler> event_handler_;
  DWORD view_port_handler_cookie_;
  base::CancelableClosure delay_update_;

  base::WeakPtrFactory<DirectManipulationHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DirectManipulationHelper);
};

}  // namespace ui

#endif  // UI_BASE_WIN_DIRECT_MANIPULATION_H_
