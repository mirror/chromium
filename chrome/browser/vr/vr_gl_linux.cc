// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/vr_gl_linux.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/vr/ui.h"
#include "chrome/browser/vr/ui_interface.h"
#include "chrome/browser/vr/ui_renderer.h"
#include "chrome/browser/vr/ui_scene.h"
#include "chrome/browser/vr/ui_scene_manager.h"
#include "chrome/browser/vr/vr_shell_renderer.h"
#include "ui/gfx/font_list.h"

// TODO(cjgrant): Sort out this mess...
#include "ui/gfx/platform_font_linux.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "base/i18n/icu_util.h"
#include "chrome/browser/vr/toolbar_state.h"
#include "components/security_state/core/security_state.h"
#include "components/toolbar/vector_icons.h"
#include "base/strings/utf_string_conversions.h"

namespace vr {

VrGlLinux::VrGlLinux() :
    ui_(base::MakeUnique<Ui>(this, this, UiInitialState())) {

  //ui::ResourceBundle::InitSharedInstanceWithPakPath(
      //pak_path.AppendASCII("components_tests_resources.pak"));

  auto resource_bundle = ResourceBundle::InitSharedInstanceWithLocale(
        "en-US", nullptr, ResourceBundle::LOAD_COMMON_RESOURCES);

  ui::RegisterPathProvider();
  base::FilePath pak_path;
  PathService::Get(base::DIR_MODULE, &pak_path);
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
      pak_path.AppendASCII("components_tests_resources.pak"),
      ui::ScaleFactor::SCALE_FACTOR_NONE);

  //gfx::FontList::SetDefaultFontDescription("");

  base::i18n::InitializeICU();

  GURL gurl("https://dangerous.com/dir/file.html");
  ToolbarState state(gurl, security_state::SecurityLevel::DANGEROUS,
                     &toolbar::kHttpsInvalidIcon,
                     base::UTF8ToUTF16("Not secure"),
                     true, false);
  ui_->browser_ui()->SetToolbarState(state);

  ui_->OnGlInitialized(0);
}

VrGlLinux::~VrGlLinux() = default;

void VrGlLinux::DrawFrame(int16_t frame_index) {
  base::TimeTicks current_time = base::TimeTicks::Now();

  // device::GvrDelegate::GetGvrPoseWithNeckModel(
  // gvr_api_.get(), &render_info_primary_.head_pose);

  // Update the render position of all UI elements (including desktop).
  const gfx::Transform head_pose;
  ui_->scene()->OnBeginFrame(current_time, gfx::Vector3dF(0.f, 0.f, -1.0f));
  // GetForwardVector(head_pose));

  // UpdateController(render_info_primary_.head_pose);

  // Ensure that all elements are ready before drawing. Eg., elements may have
  // been dirtied due to animation on input processing and need to regenerate
  // textures.
  ui_->scene()->PrepareToDraw();

  // UpdateEyeInfos(render_info_primary_.head_pose, kViewportListPrimaryOffset,
  // render_info_primary_.surface_texture_size,
  // &render_info_primary_);

  // Measure projected content size and bubble up if delta exceeds threshold.
  // browser_->OnProjMatrixChanged(
  //    render_info_primary_.left_eye_info.proj_matrix);

  ControllerInfo controller_info;
  controller_info.target_point = {0, 0, -1};
  controller_info.target_point = {0, 0, 0};
  controller_info.transform = head_pose;
  controller_info.opacity = 1.0f;
  controller_info.reticle_render_target = nullptr;

  const gfx::Transform view_proj(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, -1,
                                 0);

  RenderInfo render_info;
  render_info.head_pose = head_pose;  // identity
  // render_info.surface_texture_size = {400, 400}; // Anroid: 2658x1590
  render_info.surface_texture_size = {1024, 768};
  // render_info.left_eye_info.viewport = {0, 0, 200, 400}; // 1329x1590
  render_info.left_eye_info.viewport = {0, 0, 1024, 768};
  render_info.left_eye_info.view_matrix = head_pose;  // identity.

  render_info.left_eye_info.proj_matrix = view_proj;
  render_info.left_eye_info.view_proj_matrix = view_proj;
  render_info.right_eye_info = render_info.left_eye_info;
  render_info.right_eye_info.viewport = {0, 0, 0, 0};

  ui_->ui_renderer()->Draw(render_info, controller_info);

  //if (!scene_->GetViewportAwareElements().empty() && !ShouldDrawWebVr()) {
    //ui_->ui_renderer()->DrawViewportAware(render_info, controller_info, false);
  //}
}

void VrGlLinux::OnContentEnter(const gfx::PointF& normalized_hit_point) {}

void VrGlLinux::OnContentLeave() {}

void VrGlLinux::OnContentMove(const gfx::PointF& normalized_hit_point) {}

void VrGlLinux::OnContentDown(const gfx::PointF& normalized_hit_point) {}

void VrGlLinux::OnContentUp(const gfx::PointF& normalized_hit_point) {}

void VrGlLinux::OnContentFlingStart(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}

void VrGlLinux::OnContentFlingCancel(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}

void VrGlLinux::OnContentScrollBegin(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}

void VrGlLinux::OnContentScrollUpdate(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}

void VrGlLinux::OnContentScrollEnd(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}

void VrGlLinux::ExitPresent() {}
void VrGlLinux::ExitFullscreen() {}
void VrGlLinux::NavigateBack() {}
void VrGlLinux::ExitCct() {}
void VrGlLinux::OnUnsupportedMode(vr::UiUnsupportedMode mode) {}
void VrGlLinux::OnExitVrPromptResult(vr::UiUnsupportedMode reason,
                                     vr::ExitVrPromptChoice choice) {}
void VrGlLinux::OnContentScreenBoundsChanged(const gfx::SizeF& bounds) {}

}  // namespace vr
