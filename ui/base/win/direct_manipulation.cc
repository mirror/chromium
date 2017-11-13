// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/direct_manipulation.h"

#include <objbase.h>
#include <iomanip>
#include <string>

#include "base/win/windows_version.h"

namespace ui {

void error(bool statement) {
  if (!statement) {
    LOG(ERROR) << GetLastError();
  }

  // let it crash.
  DCHECK(statement);
}

// static
std::unique_ptr<DirectManipulationHelper>
DirectManipulationHelper::CreateInstance() {
  std::unique_ptr<DirectManipulationHelper> instance;

  if (base::win::GetVersion() >= base::win::VERSION_WIN10)
    instance.reset(new DirectManipulationHelper);

  return instance;
}

DirectManipulationHelper::DirectManipulationHelper() {}

DirectManipulationHelper::~DirectManipulationHelper() {
  if (view_port_outer_)
    view_port_outer_->Abandon();
}

void DirectManipulationHelper::Initialize(HWND window,
                                          WindowEventTarget* event_target) {
  error(::IsWindow(window));

  // TODO(ananta)
  // Remove the CHECK statements here and below and replace them with logs
  // when this code stabilizes.

  // The IDirectManipulationManager is the main entrypoint of the DManip API
  // surface. We use it to get other objects, create "viewports", and activate
  // DManip processing.
  HRESULT hr =
      ::CoCreateInstance(CLSID_DirectManipulationManager, nullptr,
                         CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&manager_));
  error(SUCCEEDED(hr));

  // The IDirectManipulationUpdateManager is how we inform DManip that we are
  // "rendering" a frame - when we do so, we'll get callbacks for any active
  // manipulations on our viewports.
  hr = manager_->GetUpdateManager(IID_PPV_ARGS(&update_manager_));
  error(SUCCEEDED(hr));

  // The IDirectManipulationUpdateManager is how we inform DManip that we are
  // "rendering" a frame - when we do so, we'll get callbacks for any active
  // manipulations on our viewports.
  hr = manager_->CreateViewport(nullptr, window,
                                IID_PPV_ARGS(&view_port_outer_));
  error(SUCCEEDED(hr));

  // The IDirectManipulationUpdateManager is how we inform DManip that we are
  // "rendering" a frame - when we do so, we'll get callbacks for any active
  // manipulations on our viewports.
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
  error(SUCCEEDED(hr));

  // By specifying manual update, we take on the responsibility of calling
  // Update() on each "render tick", rather than just during inertia. It also
  // means that during a running manipulation, DManip will wait until we call
  // Update() to tell us of any generated motion, rather than queuing a
  // notification from the input event itself.
  hr = view_port_outer_->SetViewportOptions(
      DIRECTMANIPULATION_VIEWPORT_OPTIONS_MANUALUPDATE);
  error(SUCCEEDED(hr));

  event_handler_ = Microsoft::WRL::Make<DirectManipulationHandler>();
  event_handler_->SetDirectManipulationHelper(this);
  event_handler_->SetWindowEventTarget(event_target);
  error(event_handler_);

  // The CViewportEventHandler is where we'll actually get the callbacks from
  // DManip that tell us the viewport's state as well as how the content has
  // been manipulated.
  hr = view_port_outer_->AddEventHandler(window, event_handler_.Get(),
                                         &view_port_handler_cookie_);
  error(SUCCEEDED(hr));

  hr = view_port_outer_->SetViewportRect(&default_rect_);
  error(SUCCEEDED(hr));

  manager_->Activate(window);

  // Enable the viewport so that DManip will allow it to be manipulated.
  hr = view_port_outer_->Enable();
  error(SUCCEEDED(hr));

  render_timer_ = 1;
  SetTimer(window, render_timer_, USER_TIMER_MINIMUM, nullptr);
}

void DirectManipulationHelper::SetBounds(const gfx::Rect& bounds) {
  // Set the default viewport rect. Whenever a manipulation completes, we'll
  // reset the content back here. In normal scenarios, we'd take more care to
  // set up the viewport rect, since it controls how input chains between
  // viewports and how to translate from client coordinates to viewport
  // coordinates. However, since we don't have any visual content, or other
  // viewports, we can skip that.
  // RECT rect = bounds.ToRECT();
  // HRESULT hr = view_port_outer_->SetViewportRect(&rect);
  // error(SUCCEEDED(hr));
}

void DirectManipulationHelper::Activate(HWND window) {
  // error(::IsWindow(window));
  manager_->Activate(window);

  // // Enable the viewport so that DManip will allow it to be manipulated.
  // HRESULT hr = view_port_outer_->Enable();
  // error(SUCCEEDED(hr));

  // render_timer_ = 1;
  // SetTimer(window, render_timer_, USER_TIMER_MINIMUM, nullptr);
}

void DirectManipulationHelper::Deactivate(HWND window) {
  // error(::IsWindow(window));
  manager_->Deactivate(window);
}

void DirectManipulationHelper::OnPointerHitTest(WPARAM w_param) {
  // Since WM_POINTER is not exposed for PTP, DManip can't forward over the
  // WM_POINTERDOWN message to allow the application to perform its
  // hit-test. Instead, it sends DM_POINTERHITTEST, with wParam/lParam
  // corresponding to the same info that is in pointer messages. No other
  // pointer type sends this message today, but it's best practice to check
  // that it is indeed a PTP pointer. Since this switch-conditional looks at
  // both messages, you can test with touch by adjusting the pointer type
  // check. We'd also in theory do a hit-test to see which viewport (if any)
  // to call SetContact() on (you can get the point from the pointer info or
  // GET_X_LPARAM/GET_Y_LPARAM), but since we want all PTP input to get sent
  // to DManip, regardless of its location, we will just unconditionally
  // call SetContact() on our viewport.
  UINT32 pointerId = GET_POINTERID_WPARAM(w_param);
  POINTER_INFO info = {};
  if (GetPointerInfo(pointerId, &info) && info.pointerType == PT_TOUCHPAD) {
    view_port_outer_->SetContact(pointerId);
    return;
  }
}

void DirectManipulationHelper::ResetViewport() {
  LOG(ERROR) << "Reset";
  // By zooming the primary content to a rect that matches the viewport rect,
  // we reset the content's transform to identity.
  RECT rect;
  HRESULT hr = view_port_outer_->GetViewportRect(&rect);
  error(SUCCEEDED(hr));

  hr = view_port_outer_->ZoomToRect(
      static_cast<float>(rect.left), static_cast<float>(rect.top),
      static_cast<float>(rect.right), static_cast<float>(rect.bottom), FALSE);
  error(SUCCEEDED(hr));
}

void DirectManipulationHelper::OnTimer(WPARAM w_param) {
  // LOG(ERROR) << "Timer";
  if (w_param == render_timer_)
    update_manager_->Update(nullptr);
}

HRESULT DirectManipulationHandler::OnViewportStatusChanged(
    IDirectManipulationViewport* viewport,
    DIRECTMANIPULATION_STATUS current,
    DIRECTMANIPULATION_STATUS previous) {
  // The state of our viewport has changed! We'l be in one of three states:
  // * ENABLED: initial state
  // * READY: the previous manipulation has been completed
  // * RUNNING: there are active contacts performing a manipulation
  // * INERTIA: the contacts have lifted, but they have produced residual motion

  std::string names[] = {"BUILDING", "ENABLED", "DISABLED", "RUNNING",
                         "INERTIA",  "READY",   "SUSPENDED"};
  LOG(ERROR) << "STATUS: " << names[current] << "," << helper_;

  // Reset the viewport when we're idle, so the content transforms always start
  // at identity.
  if (current == DIRECTMANIPULATION_READY ||
      current == DIRECTMANIPULATION_ENABLED) {
    helper_->ResetViewport();
    last_scale_ = 1.0f;
    last_x_offset_ = 0.0f;
    last_y_offset_ = 0.0f;
  }

  return S_OK;
}

HRESULT DirectManipulationHandler::OnViewportUpdated(
    IDirectManipulationViewport* viewport) {
  // Fired when all content in a viewport has been updated.
  // Nothing to do here.
  return S_OK;
}

HRESULT DirectManipulationHandler::OnContentUpdated(
    IDirectManipulationViewport* viewport,
    IDirectManipulationContent* content) {
  // Our content has been updated! Query its new transform.
  // For a pan, we'll get x/y translation (potentially railed).
  // For a zoom, we'll get scale as well as x/y translation, since
  // DManip is trying to keep the center point stable onscreen.
  // It can be ignored, since during a zoom we just care about how
  // the scale itself is changing.
  std::string names[] = {"BUILDING", "ENABLED", "DISABLED", "RUNNING",
                         "INERTIA",  "READY",   "SUSPENDED"};

  float xform[6];
  HRESULT hr = content->GetContentTransform(xform, ARRAYSIZE(xform));
  error(SUCCEEDED(hr));

  DIRECTMANIPULATION_STATUS status;
  hr = viewport->GetStatus(&status);
  error(SUCCEEDED(hr));

  LOG(ERROR) << std::setprecision(8) << "scale=" << xform[0] << "," << xform[3]
             << " xlate=" << xform[4] << "," << xform[5] << ","
             << names[status];

  if (event_target_)
    hr = event_target_->HandleDManipMessage(xform[0] / last_scale_,
                                            xform[4] - last_x_offset_,
                                            xform[5] - last_y_offset_);

  last_scale_ = xform[0];
  last_x_offset_ = xform[4];
  last_y_offset_ = xform[5];

  return hr;
}

void DirectManipulationHandler::SetDirectManipulationHelper(
    DirectManipulationHelper* helper) {
  helper_ = helper;
}

void DirectManipulationHandler::SetWindowEventTarget(
    WindowEventTarget* event_target) {
  event_target_ = event_target;
}

}  // namespace ui.
