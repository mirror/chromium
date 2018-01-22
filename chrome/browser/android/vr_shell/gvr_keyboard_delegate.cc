// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/gvr_keyboard_delegate.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/android/vr_shell/gvr_util.h"
#include "chrome/browser/vr/model/camera_model.h"
#include "chrome/browser/vr/model/text_input_info.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/gvr-android-sdk/src/libraries/headers/vr/gvr/capi/include/gvr.h"
#include "third_party/gvr-android-sdk/src/libraries/headers/vr/gvr/capi/include/gvr_types.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace vr_shell {

namespace {

// We have a separate pointer that's used on the thread that creates/destroys
// the keyboard so that we can properly handle destruction calls that happen
// while the keyboard is being constructed and gvr_keyboard_ is null.
static gvr_keyboard_context* keyboard_ptr_for_creation_task = nullptr;

void OnKeyboardEvent(void* closure, GvrKeyboardDelegate::EventType event) {
  auto* callback =
      reinterpret_cast<GvrKeyboardDelegate::OnEventCallback*>(closure);
  if (callback)
    callback->Run(event);
}

void CreateKeyboardTask(
    void* callback,
    base::WeakPtr<GvrKeyboardDelegate> weak_delegate,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  if (!load_gvr_sdk() || keyboard_ptr_for_creation_task)
    return;

  // This call is blocking and takes about 0.5s.
  auto* keyboard = gvr_keyboard_create(callback, OnKeyboardEvent);
  if (keyboard) {
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(&GvrKeyboardDelegate::OnKeyboardCreated,
                                  weak_delegate, base::Unretained(keyboard)));
    keyboard_ptr_for_creation_task = keyboard;
  }
}

void DestroyKeyboardTask() {
  if (!keyboard_ptr_for_creation_task)
    return;

  gvr_keyboard_destroy(&keyboard_ptr_for_creation_task);
  close_gvr_sdk();
}

}  // namespace

std::unique_ptr<GvrKeyboardDelegate> GvrKeyboardDelegate::Create() {
  return base::WrapUnique(new GvrKeyboardDelegate());
}

GvrKeyboardDelegate::GvrKeyboardDelegate() : weak_ptr_factory_(this) {
  keyboard_event_callback_ = base::BindRepeating(
      &GvrKeyboardDelegate::OnGvrKeyboardEvent, base::Unretained(this));
  CreateKeyboard();
}

GvrKeyboardDelegate::~GvrKeyboardDelegate() {
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::BindOnce(DestroyKeyboardTask));
  gvr_keyboard_ = nullptr;
}

void GvrKeyboardDelegate::OnKeyboardCreated(gvr_keyboard_context* context) {
  gvr_keyboard_ = context;
  creating_keyboard_ = false;
  if (!gvr_keyboard_)
    return;

  gvr_mat4f matrix;
  gvr_keyboard_get_recommended_world_from_keyboard_matrix(2.0f, &matrix);
  gvr_keyboard_set_world_from_keyboard_matrix(gvr_keyboard_, &matrix);

  if (pending_show_keyboard_)
    gvr_keyboard_show(gvr_keyboard_);

  pending_show_keyboard_ = false;
}

void GvrKeyboardDelegate::CreateKeyboard() {
  if (gvr_keyboard_ || creating_keyboard_)
    return;

  void* callback = reinterpret_cast<void*>(&keyboard_event_callback_);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(CreateKeyboardTask, base::Unretained(callback),
                     weak_ptr_factory_.GetWeakPtr(),
                     base::ThreadTaskRunnerHandle::Get()));
}

void GvrKeyboardDelegate::RenderingEnabled(bool enabled) {
  // The GVR keyboard eats up memory while it's loaded, so we may want to
  // destory it when its not used (see b/72056929). The reason we don't do it
  // is because re-creating results in dropped frames, perhaps due to cpu
  // intensive work in the gvr keyboard code.
}

void GvrKeyboardDelegate::SetUiInterface(vr::KeyboardUiInterface* ui) {
  ui_ = ui;
}

void GvrKeyboardDelegate::OnBeginFrame() {
  if (!gvr_keyboard_)
    return;

  gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
  gvr_keyboard_set_frame_time(gvr_keyboard_, &target_time);
  gvr_keyboard_advance_frame(gvr_keyboard_);
}

void GvrKeyboardDelegate::ShowKeyboard() {
  DCHECK(gvr_keyboard_ || creating_keyboard_);
  if (gvr_keyboard_) {
    gvr_keyboard_show(gvr_keyboard_);
  } else if (creating_keyboard_) {
    pending_show_keyboard_ = true;
  }
}

void GvrKeyboardDelegate::HideKeyboard() {
  if (gvr_keyboard_)
    gvr_keyboard_hide(gvr_keyboard_);
  pending_show_keyboard_ = false;
}

void GvrKeyboardDelegate::SetTransform(const gfx::Transform& transform) {
  if (!gvr_keyboard_)
    return;

  gvr_mat4f matrix;
  TransformToGvrMat(transform, &matrix);
  gvr_keyboard_set_world_from_keyboard_matrix(gvr_keyboard_, &matrix);
}

bool GvrKeyboardDelegate::HitTest(const gfx::Point3F& ray_origin,
                                  const gfx::Point3F& ray_target,
                                  gfx::Point3F* hit_position) {
  if (!gvr_keyboard_)
    return false;

  gvr_vec3f start;
  start.x = ray_origin.x();
  start.y = ray_origin.y();
  start.z = ray_origin.z();
  gvr_vec3f end;
  end.x = ray_target.x();
  end.y = ray_target.y();
  end.z = ray_target.z();
  gvr_vec3f hit_point;
  bool hits = gvr_keyboard_update_controller_ray(gvr_keyboard_, &start, &end,
                                                 &hit_point);
  if (hits)
    hit_position->SetPoint(hit_point.x, hit_point.y, hit_point.z);
  return hits;
}

void GvrKeyboardDelegate::Draw(const vr::CameraModel& model) {
  if (!gvr_keyboard_)
    return;

  int eye = model.eye_type;
  gvr::Mat4f view_matrix;
  TransformToGvrMat(model.view_matrix, &view_matrix);
  gvr_keyboard_set_eye_from_world_matrix(gvr_keyboard_, eye, &view_matrix);

  gvr::Mat4f proj_matrix;
  TransformToGvrMat(model.proj_matrix, &proj_matrix);
  gvr_keyboard_set_projection_matrix(gvr_keyboard_, eye, &proj_matrix);

  gfx::Rect viewport_rect = model.viewport;
  const gvr::Recti viewport = {viewport_rect.x(), viewport_rect.right(),
                               viewport_rect.y(), viewport_rect.bottom()};
  gvr_keyboard_set_viewport(gvr_keyboard_, eye, &viewport);
  gvr_keyboard_render(gvr_keyboard_, eye);
}

void GvrKeyboardDelegate::OnButtonDown(const gfx::PointF& position) {
  if (!gvr_keyboard_)
    return;

  gvr_keyboard_update_button_state(
      gvr_keyboard_, gvr::ControllerButton::GVR_CONTROLLER_BUTTON_CLICK, true);
}

void GvrKeyboardDelegate::OnButtonUp(const gfx::PointF& position) {
  if (!gvr_keyboard_)
    return;

  gvr_keyboard_update_button_state(
      gvr_keyboard_, gvr::ControllerButton::GVR_CONTROLLER_BUTTON_CLICK, false);
}

void GvrKeyboardDelegate::UpdateInput(const vr::TextInputInfo& info) {
  if (!gvr_keyboard_)
    return;

  gvr_keyboard_set_text(gvr_keyboard_, base::UTF16ToUTF8(info.text).c_str());
  gvr_keyboard_set_selection_indices(gvr_keyboard_, info.selection_start,
                                     info.selection_end);
}

void GvrKeyboardDelegate::OnGvrKeyboardEvent(EventType event) {
  DCHECK(ui_ != nullptr);
  switch (event) {
    case GVR_KEYBOARD_ERROR_UNKNOWN:
      LOG(ERROR) << "Unknown GVR keyboard error.";
      break;
    case GVR_KEYBOARD_ERROR_SERVICE_NOT_CONNECTED:
      LOG(ERROR) << "GVR keyboard service not connected.";
      break;
    case GVR_KEYBOARD_ERROR_NO_LOCALES_FOUND:
      LOG(ERROR) << "No GVR keyboard locales found.";
      break;
    case GVR_KEYBOARD_ERROR_SDK_LOAD_FAILED:
      LOG(ERROR) << "GVR keyboard sdk load failed.";
      break;
    case GVR_KEYBOARD_SHOWN:
      break;
    case GVR_KEYBOARD_HIDDEN:
      ui_->OnKeyboardHidden();
      break;
    case GVR_KEYBOARD_TEXT_UPDATED:
      ui_->OnInputEdited(GetTextInfo());
      break;
    case GVR_KEYBOARD_TEXT_COMMITTED:
      ui_->OnInputCommitted(GetTextInfo());
      break;
  }
}

vr::TextInputInfo GvrKeyboardDelegate::GetTextInfo() {
  vr::TextInputInfo info;
  // Get text. Note that we wrap the text in a unique ptr since we're
  // responsible for freeing the memory allocated by gvr_keyboard_get_text.
  std::unique_ptr<char, decltype(std::free)*> scoped_text{
      gvr_keyboard_get_text(gvr_keyboard_), std::free};
  std::string text(scoped_text.get());
  info.text = base::UTF8ToUTF16(text);
  // Get selection indices.
  size_t start, end;
  gvr_keyboard_get_selection_indices(gvr_keyboard_, &start, &end);
  info.selection_start = start;
  info.selection_end = end;
  return info;
}

}  // namespace vr_shell
