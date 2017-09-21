// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/vr_gl_thread.h"

#include <utility>

#include "base/message_loop/message_loop.h"
#include "chrome/browser/android/vr_shell/vr_input_manager.h"
#include "chrome/browser/android/vr_shell/vr_shell.h"
#include "chrome/browser/android/vr_shell/vr_shell_gl.h"
#include "chrome/browser/vr/toolbar_state.h"
#include "chrome/browser/vr/ui_interface.h"
#include "chrome/browser/vr/ui_scene.h"
#include "chrome/browser/vr/ui_scene_manager.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace vr_shell {

VrGLThread::VrGLThread(
    const base::WeakPtr<VrShell>& weak_vr_shell,
    scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner,
    gvr_context* gvr_api,
    bool initially_web_vr,
    bool web_vr_autopresentation_expected,
    bool in_cct,
    bool reprojected_rendering,
    bool daydream_support)
    : base::android::JavaHandlerThread("VrShellGL"),
      weak_vr_shell_(weak_vr_shell),
      main_thread_task_runner_(std::move(main_thread_task_runner)),
      gvr_api_(gvr_api),
      initially_web_vr_(initially_web_vr),
      web_vr_autopresentation_expected_(web_vr_autopresentation_expected),
      in_cct_(in_cct),
      reprojected_rendering_(reprojected_rendering),
      daydream_support_(daydream_support) {}

VrGLThread::~VrGLThread() {
  Stop();
}

base::WeakPtr<VrShellGl> VrGLThread::GetVrShellGl() {
  return vr_shell_gl_->GetWeakPtr();
}

void VrGLThread::Init() {
  vr::UiInitialState ui_state;
  ui_state.in_cct = in_cct_;
  ui_state.in_web_vr = initially_web_vr_;
  ui_state.web_vr_autopresentation_expected = web_vr_autopresentation_expected_;

  vr_shell_gl_ =
      base::MakeUnique<VrShellGl>(this, this, ui_state, gvr_api_,
                                  reprojected_rendering_, daydream_support_);

  ui_ = vr_shell_gl_->GetUiWeakPtr();
  vr_shell_gl_->Initialize();
}

void VrGLThread::CleanUp() {
  vr_shell_gl_.reset();
}

void VrGLThread::ContentSurfaceChanged(jobject surface) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&VrShell::ContentSurfaceChanged, weak_vr_shell_, surface));
}

void VrGLThread::GvrDelegateReady(gvr::ViewerType viewer_type) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&VrShell::GvrDelegateReady, weak_vr_shell_, viewer_type));
}

void VrGLThread::UpdateGamepadData(device::GvrGamepadData pad) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::UpdateGamepadData, weak_vr_shell_, pad));
}

void VrGLThread::ProcessContentGesture(
    std::unique_ptr<blink::WebInputEvent> event) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::ProcessContentGesture, weak_vr_shell_,
                            base::Passed(std::move(event))));
}

void VrGLThread::ForceExitVr() {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::ForceExitVr, weak_vr_shell_));
}

void VrGLThread::ExitPresent() {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::ExitPresent, weak_vr_shell_));
}

void VrGLThread::ExitFullscreen() {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::ExitFullscreen, weak_vr_shell_));
}

void VrGLThread::OnContentPaused(bool enabled) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&VrShell::OnContentPaused, weak_vr_shell_, enabled));
}

void VrGLThread::NavigateBack() {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::NavigateBack, weak_vr_shell_));
}

void VrGLThread::ExitCct() {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::ExitCct, weak_vr_shell_));
}

void VrGLThread::ToggleCardboardGamepad(bool enabled) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&VrShell::ToggleCardboardGamepad, weak_vr_shell_, enabled));
}

void VrGLThread::OnUnsupportedMode(vr::UiUnsupportedMode mode) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::OnUnsupportedMode, weak_vr_shell_, mode));
}

void VrGLThread::OnExitVrPromptResult(vr::UiUnsupportedMode reason,
                                      vr::ExitVrPromptChoice choice) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::OnExitVrPromptResult, weak_vr_shell_,
                            reason, choice));
}

void VrGLThread::OnContentScreenBoundsChanged(const gfx::SizeF& bounds) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VrShell::OnContentScreenBoundsChanged,
                            weak_vr_shell_, bounds));
}

void VrGLThread::SetFullscreen(bool enabled) {
  task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&vr::BrowserUiInterface::SetFullscreen, ui_, enabled));
}

void VrGLThread::SetIncognito(bool incognito) {
  task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&vr::BrowserUiInterface::SetIncognito, ui_, incognito));
}

void VrGLThread::SetHistoryButtonsEnabled(bool can_go_back,
                                          bool can_go_forward) {
  task_runner()->PostTask(
      FROM_HERE, base::Bind(&vr::BrowserUiInterface::SetHistoryButtonsEnabled,
                            ui_, can_go_back, can_go_forward));
}

void VrGLThread::SetLoadProgress(float progress) {
  task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&vr::BrowserUiInterface::SetLoadProgress, ui_, progress));
}

void VrGLThread::SetLoading(bool loading) {
  task_runner()->PostTask(
      FROM_HERE, base::Bind(&vr::BrowserUiInterface::SetLoading, ui_, loading));
}

void VrGLThread::SetToolbarState(const vr::ToolbarState& state) {
  task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&vr::BrowserUiInterface::SetToolbarState, ui_, state));
}

void VrGLThread::SetWebVrMode(bool enabled, bool show_toast) {
  task_runner()->PostTask(
      FROM_HERE, base::Bind(&vr::BrowserUiInterface::SetWebVrMode, ui_, enabled,
                            show_toast));
}

void VrGLThread::SetWebVrSecureOrigin(bool secure) {
  task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&vr::BrowserUiInterface::SetWebVrSecureOrigin, ui_, secure));
}

void VrGLThread::SetAudioCapturingIndicator(bool enabled) {
  task_runner()->PostTask(
      FROM_HERE, base::Bind(&vr::BrowserUiInterface::SetAudioCapturingIndicator,
                            ui_, enabled));
}

void VrGLThread::SetLocationAccessIndicator(bool enabled) {
  task_runner()->PostTask(
      FROM_HERE, base::Bind(&vr::BrowserUiInterface::SetLocationAccessIndicator,
                            ui_, enabled));
}

void VrGLThread::SetVideoCapturingIndicator(bool enabled) {
  task_runner()->PostTask(
      FROM_HERE, base::Bind(&vr::BrowserUiInterface::SetVideoCapturingIndicator,
                            ui_, enabled));
}

void VrGLThread::SetScreenCapturingIndicator(bool enabled) {
  task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&vr::BrowserUiInterface::SetScreenCapturingIndicator, ui_,
                 enabled));
}

void VrGLThread::SetBluetoothConnectedIndicator(bool enabled) {
  task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&vr::BrowserUiInterface::SetBluetoothConnectedIndicator, ui_,
                 enabled));
}

void VrGLThread::SetIsExiting() {
  task_runner()->PostTask(
      FROM_HERE, base::Bind(&vr::BrowserUiInterface::SetIsExiting, ui_));
}

void VrGLThread::SetExitVrPromptEnabled(bool enabled,
                                        vr::UiUnsupportedMode reason) {
  task_runner()->PostTask(
      FROM_HERE, base::Bind(&vr::BrowserUiInterface::SetExitVrPromptEnabled,
                            ui_, enabled, reason));
}

}  // namespace vr_shell
