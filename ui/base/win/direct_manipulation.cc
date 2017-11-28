// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/direct_manipulation.h"

#include <objbase.h>
#include <cmath>
#include <iomanip>
#include <limits>
#include <string>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/windows_version.h"

namespace ui {

const base::Feature kPrecisionTouchpad{"PrecisionTouchpad",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

// static
std::unique_ptr<DirectManipulationHelper>
DirectManipulationHelper::CreateInstance() {
  // DM_POINTERHITTEST supported since Win10.
  if (base::FeatureList::IsEnabled(kPrecisionTouchpad) &&
      base::win::GetVersion() >= base::win::VERSION_WIN10)
    return base::WrapUnique(new DirectManipulationHelper());

  return nullptr;
}

DirectManipulationHelper::DirectManipulationHelper() : weak_factory_(this) {}

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
  DCHECK(event_handler_);
  event_handler_->Initialize(this, event_target);

  // We got Direct Manipulation transform from
  // IDirectManipulationViewportEventHandler.
  hr = view_port_outer_->AddEventHandler(window, event_handler_.Get(),
                                         &view_port_handler_cookie_);
  DCHECK(SUCCEEDED(hr));

  hr = view_port_outer_->SetViewportRect(&default_rect_);
  DCHECK(SUCCEEDED(hr));

  manager_->Activate(window);

  hr = view_port_outer_->Enable();
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

void DirectManipulationHelper::OnPointerHitTest(WPARAM w_param) {
  // Only DM_POINTERHITTEST can be the first message of input sequence of
  // touchpad input.
  // TODO(chaopeng) Check if Windows API changes:
  // For WM_POINTER, the pointer type will show the event from mouse.
  // For WM_POINTERACTIVATE, the pointer id will be different with the following
  // message.
  UINT32 pointerId = GET_POINTERID_WPARAM(w_param);
  POINTER_INFO info = {};
  if (GetPointerInfo(pointerId, &info) && info.pointerType == PT_TOUCHPAD) {
    view_port_outer_->SetContact(pointerId);
    // Start the timer for fake viewport.
    StartTimer();
    return;
  }
}

void DirectManipulationHelper::ResetViewport() {
  RECT rect;
  HRESULT hr = view_port_outer_->GetViewportRect(&rect);
  DCHECK(SUCCEEDED(hr));

  hr = view_port_outer_->ZoomToRect(
      static_cast<float>(rect.left), static_cast<float>(rect.top),
      static_cast<float>(rect.right), static_cast<float>(rect.bottom), FALSE);
  DCHECK(SUCCEEDED(hr));

  StopTimer();
}

void DirectManipulationHelper::Update() {
  // Simulate 1 frame in update_manager_.
  update_manager_->Update(nullptr);

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, delay_update_.callback(),
      base::TimeDelta::FromSecondsD(1. / 120));
}

void DirectManipulationHelper::StartTimer() {
  delay_update_.Reset(base::Bind(&DirectManipulationHelper::Update,
                                 weak_factory_.GetWeakPtr()));

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, delay_update_.callback(),
      base::TimeDelta::FromSecondsD(1. / 120));
}

void DirectManipulationHelper::StopTimer() {
  delay_update_.Cancel();
}

void DirectManipulationHandler::Initialize(DirectManipulationHelper* helper,
                                           WindowEventTarget* event_target) {
  DCHECK(helper);
  DCHECK(event_target);
  helper_ = helper;
  event_target_ = event_target;
  pinch_begin_sent_ = false;
  last_scale_ = 1.0f;
  last_x_offset_ = 0.0f;
  last_y_offset_ = 0.0f;
}

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
  if (current == DIRECTMANIPULATION_READY ||
      current == DIRECTMANIPULATION_ENABLED) {
    helper_->ResetViewport();
    last_scale_ = 1.0f;
    last_x_offset_ = 0.0f;
    last_y_offset_ = 0.0f;

    if (pinch_begin_sent_) {
      pinch_begin_sent_ = false;
      event_target_->SendGesturePinchEnd();
    }
  }

  return S_OK;
}

HRESULT DirectManipulationHandler::OnViewportUpdated(
    IDirectManipulationViewport* viewport) {
  // Nothing to do here.
  return S_OK;
}

bool FloatEquals(float a, float b) {
  return fabs(a - b) < std::numeric_limits<float>::epsilon();
}

HRESULT DirectManipulationHandler::OnContentUpdated(
    IDirectManipulationViewport* viewport,
    IDirectManipulationContent* content) {
  float xform[6];
  HRESULT hr = content->GetContentTransform(xform, ARRAYSIZE(xform));
  DCHECK(SUCCEEDED(hr));

  float scale = xform[0];
  float x_offset = xform[4];
  float y_offset = xform[5];

  // Ignore the change less than float point rounding error.
  if (FloatEquals(scale, last_scale_) &&
      FloatEquals(x_offset, last_x_offset_) &&
      FloatEquals(y_offset, last_y_offset_)) {
    return true;
  }

  // Consider this is a Scroll when scale factor equals 1.0
  if (FloatEquals(scale, 1.0)) {
    hr = event_target_->SendMouseWheel(x_offset - last_x_offset_,
                                       y_offset - last_y_offset_);
  } else {  // Pinch zoom
    if (!pinch_begin_sent_)
      event_target_->SendGesturePinchBegin();
    event_target_->SendGesturePinchUpdate(scale / last_scale_);
  }

  last_scale_ = scale;
  last_x_offset_ = x_offset;
  last_y_offset_ = y_offset;

  return hr;
}

}  // namespace ui.
