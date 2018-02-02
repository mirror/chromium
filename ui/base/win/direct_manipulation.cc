// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/direct_manipulation.h"

#include <objbase.h>
#include <cmath>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/windows_version.h"
#include "ui/base/ui_base_features.h"

namespace ui {
namespace win {

// DirectManipulationHandler receives status update and gesture events from
// Direct Manipulation API.
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
  DirectManipulationHandler();
  ~DirectManipulationHandler() override;

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
  DirectManipulationHelper* helper_ = nullptr;
  WindowEventTarget* event_target_ = nullptr;
  float last_scale_ = 1.0f;
  int last_x_offset_ = 0;
  int last_y_offset_ = 0;
  bool first_ready_ = false;

  DISALLOW_COPY_AND_ASSIGN(DirectManipulationHandler);
};

// static
std::unique_ptr<DirectManipulationHelper>
DirectManipulationHelper::CreateInstance() {
  // DM_POINTERHITTEST supported since Win10.
  if (base::FeatureList::IsEnabled(features::kPrecisionTouchpad) &&
      base::win::GetVersion() >= base::win::VERSION_WIN10)
    return base::WrapUnique(new DirectManipulationHelper());

  return nullptr;
}

DirectManipulationHelper::DirectManipulationHelper() {}

DirectManipulationHelper::~DirectManipulationHelper() {
  if (view_port_outer_)
    view_port_outer_->Abandon();
}

void DirectManipulationHelper::Initialize(HWND window,
                                          WindowEventTarget* event_target) {
  DCHECK(::IsWindow(window));
  DCHECK(event_target);

  // TODO(ananta)
  // Remove the CHECK statements here and below and replace them with logs
  // when this code stabilizes.

  // IDirectManipulationUpdateManager is the first COM object created by the
  // application to retrieve other objects in the Direct Manipulation API.
  // It also serves to activate and deactivate Direct Manipulation functionality
  // on a per-HWND basis.
  HRESULT hr =
      ::CoCreateInstance(CLSID_DirectManipulationManager, nullptr,
                         CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&manager_));
  DCHECK(SUCCEEDED(hr));

  // Since we want to use fake viewport, we need UpdateManager to tell a fake
  // fake render frame.
  hr = manager_->GetUpdateManager(IID_PPV_ARGS(&update_manager_));
  DCHECK(SUCCEEDED(hr));

  hr = manager_->CreateViewport(nullptr, window,
                                IID_PPV_ARGS(&view_port_outer_));
  DCHECK(SUCCEEDED(hr));

  DIRECTMANIPULATION_CONFIGURATION configuration =
      DIRECTMANIPULATION_CONFIGURATION_INTERACTION |
      DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X |
      DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y |
      DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_INERTIA |
      DIRECTMANIPULATION_CONFIGURATION_RAILS_X |
      DIRECTMANIPULATION_CONFIGURATION_RAILS_Y |
      DIRECTMANIPULATION_CONFIGURATION_SCALING |
      DIRECTMANIPULATION_CONFIGURATION_SCALING_INERTIA;

  hr = view_port_outer_->ActivateConfiguration(configuration);
  DCHECK(SUCCEEDED(hr));

  // Since we are using fake viewport and only want to use Direct Manipulation
  // for touchpad, we need to use MANUALUPDATE option.
  hr = view_port_outer_->SetViewportOptions(
      DIRECTMANIPULATION_VIEWPORT_OPTIONS_MANUALUPDATE);
  DCHECK(SUCCEEDED(hr));

  event_handler_ = Microsoft::WRL::Make<DirectManipulationHandler>();
  event_handler_->SetDirectManipulationHelper(this);
  event_handler_->SetWindowEventTarget(event_target);
  DCHECK(event_handler_);

  // We got Direct Manipulation transform from
  // IDirectManipulationViewportEventHandler.
  hr = view_port_outer_->AddEventHandler(window, event_handler_.Get(),
                                         &view_port_handler_cookie_);
  DCHECK(SUCCEEDED(hr));

  hr = view_port_outer_->SetViewportRect(&default_rect_);
  DCHECK(SUCCEEDED(hr));

  manager_->Activate(window);

  hr = view_port_outer_->Enable();
  update_manager_->Update(nullptr);
  DCHECK(SUCCEEDED(hr));
}

void DirectManipulationHelper::Activate(HWND window) {
  DCHECK(::IsWindow(window));
  manager_->Activate(window);
}

void DirectManipulationHelper::Deactivate(HWND window) {
  DCHECK(::IsWindow(window));
  manager_->Deactivate(window);
}

bool DirectManipulationHelper::OnPointerHitTest(WPARAM w_param) {
  // Only DM_POINTERHITTEST can be the first message of input sequence of
  // touchpad input.
  // TODO(chaopeng) Check if Windows API changes:
  // For WM_POINTER, the pointer type will show the event from mouse.
  // For WM_POINTERACTIVATE, the pointer id will be different with the following
  // message.
  using GetPointerTypeFn = BOOL(WINAPI*)(UINT32, POINTER_INPUT_TYPE*);
  UINT32 pointer_id = GET_POINTERID_WPARAM(w_param);
  POINTER_INPUT_TYPE pointer_type;
  static GetPointerTypeFn get_pointer_type = reinterpret_cast<GetPointerTypeFn>(
      GetProcAddress(GetModuleHandleA("user32.dll"), "GetPointerType"));
  if (get_pointer_type && get_pointer_type(pointer_id, &pointer_type) &&
      pointer_type == PT_TOUCHPAD) {
    view_port_outer_->SetContact(pointer_id);
    // Request begin frame for fake viewport.
    need_animtation_ = true;
  }
  return need_animtation_;
}

void DirectManipulationHelper::ResetViewport(bool need_animtation) {
  RECT rect;
  HRESULT hr = view_port_outer_->GetViewportRect(&rect);
  DCHECK(SUCCEEDED(hr));

  hr = view_port_outer_->ZoomToRect(
      static_cast<float>(rect.left), static_cast<float>(rect.top),
      static_cast<float>(rect.right), static_cast<float>(rect.bottom), FALSE);
  DCHECK(SUCCEEDED(hr));

  need_animtation_ = need_animtation;
}

bool DirectManipulationHelper::OnAnimationStep() {
  // Simulate 1 frame in update_manager_.
  update_manager_->Update(nullptr);
  return need_animtation_;
}

void DirectManipulationHelper::SetEventTarget(WindowEventTarget* event_target) {
  event_handler_->SetWindowEventTarget(event_target);
}

// DirectManipulationHandler
DirectManipulationHandler::DirectManipulationHandler() {}

DirectManipulationHandler::~DirectManipulationHandler() {}

HRESULT DirectManipulationHandler::OnViewportStatusChanged(
    IDirectManipulationViewport* viewport,
    DIRECTMANIPULATION_STATUS current,
    DIRECTMANIPULATION_STATUS previous) {
  // The state of our viewport has changed! We'l be in one of three states:
  // - ENABLED: initial state
  // - READY: the previous gesture has been completed
  // - RUNNING: gesture updating
  // - INERTIA: finger leave touchpad content still updating by inertia

  // Reset the viewport when we're idle, so the content transforms always start
  // at identity.
  if (current == DIRECTMANIPULATION_READY) {
    // Every animation will receive 2 ready message, we should stop request
    // compositor animation at the second ready.
    first_ready_ = !first_ready_;
    helper_->ResetViewport(first_ready_);
    last_scale_ = 1.0f;
    last_x_offset_ = 0.0f;
    last_y_offset_ = 0.0f;
  }

  return S_OK;
}

HRESULT DirectManipulationHandler::OnViewportUpdated(
    IDirectManipulationViewport* viewport) {
  // Nothing to do here.
  return S_OK;
}

bool FloatEquals(float f1, float f2) {
  // The idea behind this is to use this fraction of the larger of the
  // two numbers as the limit of the difference.  This breaks down near
  // zero, so we reuse this as the minimum absolute size we will use
  // for the base of the scale too.
  static const float epsilon_scale = 0.00001f;
  return fabs(f1 - f2) <
         epsilon_scale *
             std::fmax(std::fmax(std::fabs(f1), std::fabs(f2)), epsilon_scale);
}

bool DifferentLessThanOne(int f1, int f2) {
  return abs(f1 - f2) < 1;
}

HRESULT DirectManipulationHandler::OnContentUpdated(
    IDirectManipulationViewport* viewport,
    IDirectManipulationContent* content) {
  float xform[6];
  HRESULT hr = content->GetContentTransform(xform, ARRAYSIZE(xform));
  DCHECK(SUCCEEDED(hr));

  float scale = xform[0];
  int x_offset = xform[4];
  int y_offset = xform[5];

  // Ignore if Windows pass scale=0 to us.
  if (scale == 0.0f) {
    LOG(ERROR) << "Windows DirectManipulation API pass scale = 0.";
    return hr;
  }

  // Ignore the scale factor change less than float point rounding error and
  // scroll offset change less than 1.
  // TODO(456622) Because we don't fully support fractional scroll, pass float
  // scroll offset feels steppy. eg.
  // first x_offset is 0.1 ignored, but last_x_offset_ set to 0.1
  // second x_offset is 1 but x_offset - last_x_offset_ is 0.9 ignored.
  if (FloatEquals(scale, last_scale_) &&
      DifferentLessThanOne(x_offset, last_x_offset_) &&
      DifferentLessThanOne(y_offset, last_y_offset_)) {
    return hr;
  }

  DCHECK_NE(last_scale_, 0.0f);

  // Consider this is a Scroll when scale factor equals 1.0.
  if (FloatEquals(scale, 1.0f)) {
    event_target_->ApplyPanGestureScroll(x_offset - last_x_offset_,
                                         y_offset - last_y_offset_);
  } else {
    event_target_->ApplyPinchZoomScale(scale / last_scale_);
  }

  last_scale_ = scale;
  last_x_offset_ = x_offset;
  last_y_offset_ = y_offset;

  return hr;
}

void DirectManipulationHandler::SetDirectManipulationHelper(
    DirectManipulationHelper* helper) {
  DCHECK(helper);
  helper_ = helper;
}

void DirectManipulationHandler::SetWindowEventTarget(
    WindowEventTarget* event_target) {
  DCHECK(event_target);
  event_target_ = event_target;
}

}  // namespace win
}  // namespace ui
