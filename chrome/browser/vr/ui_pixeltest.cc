// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/vr/test/constants.h"
#include "chrome/browser/vr/test/gl_test_environment.h"
#include "chrome/browser/vr/test/mock_browser_interface.h"
#include "chrome/browser/vr/test/mock_content_input_delegate.h"
#include "chrome/browser/vr/test/pixeltest_utils.h"
#include "chrome/browser/vr/toolbar_state.h"
#include "chrome/browser/vr/ui.h"
#include "chrome/browser/vr/ui_browser_interface.h"
#include "chrome/browser/vr/ui_input_manager.h"
#include "chrome/browser/vr/ui_renderer.h"
#include "chrome/browser/vr/ui_scene.h"
#include "chrome/browser/vr/ui_scene_manager.h"
#include "chrome/browser/vr/vr_shell_renderer.h"
#include "components/toolbar/vector_icons.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/test/gl_image_test_template.h"

namespace vr {

namespace {

static const gfx::Transform kIdentity;

}  // namespace

TEST(UiPixeltest, SaveLeftEyeToPng) {
  // Resolution of Pixel Phone for one eye.
  const gfx::Size frame_buffer_size(960, 1080);

  GlTestEnvironment gl_test_environment(frame_buffer_size);

  // Make content texture.
  GLuint content_texture = gl::GLTestHelper::CreateTexture(GL_TEXTURE_2D);
  // TODO(tiborg): Make GL_TEXTURE_EXTERNAL_OES texture for content and fill it
  // with fake content.
  ASSERT_EQ(glGetError(), (GLenum)GL_NO_ERROR);

  // Set up scene.
  MockBrowserInterface mock_browser;
  MockContentInputDelegate mock_content_input_delegate;
  vr::UiInitialState ui_initial_state;
  ui_initial_state.in_cct = false;
  ui_initial_state.in_web_vr = false;
  ui_initial_state.web_vr_autopresentation_expected = false;
  auto ui = base::MakeUnique<Ui>(&mock_browser, &mock_content_input_delegate,
                                 ui_initial_state);
  ui->OnGlInitialized(content_texture);
  ui->GetBrowserUiWeakPtr()->SetToolbarState(ToolbarState(
      GURL("https://example.com"), security_state::SECURE,
      &toolbar::kHttpsValidIcon, base::UTF8ToUTF16("Secure"), true, false));

  // Render UI.
  ControllerInfo controller_info;
  controller_info.transform = kIdentity;
  controller_info.opacity = 1.0f;
  controller_info.laser_origin = gfx::Point3F(0.0f, 0.0f, 0.0f);
  controller_info.touchpad_button_state = UiInputManager::ButtonState::UP;
  controller_info.app_button_state = UiInputManager::ButtonState::UP;
  controller_info.home_button_state = UiInputManager::ButtonState::UP;
  RenderInfo render_info;
  render_info.head_pose = kIdentity;
  render_info.surface_texture_size = frame_buffer_size;
  render_info.left_eye_info.viewport = {0, 0, frame_buffer_size.width(),
                                        frame_buffer_size.height()};
  render_info.left_eye_info.view_matrix = kIdentity;

  render_info.left_eye_info.proj_matrix = kProjMatrix;
  render_info.left_eye_info.view_proj_matrix = kProjMatrix * kIdentity;
  render_info.right_eye_info = render_info.left_eye_info;
  render_info.right_eye_info.viewport = {0, 0, 0, 0};
  GestureList gesture_list;

  ui->input_manager()->HandleInput(
      gfx::Vector3dF(0.0f, 0.0f, -1.0f), controller_info.laser_origin,
      controller_info.touchpad_button_state, &gesture_list,
      &controller_info.target_point, &controller_info.reticle_render_target);
  ui->scene()->OnBeginFrame(
      base::TimeTicks(),
      gfx::Vector3dF(-render_info.head_pose.matrix().get(2, 0),
                     -render_info.head_pose.matrix().get(2, 1),
                     -render_info.head_pose.matrix().get(2, 2)));
  ui->scene()->PrepareToDraw();
  ui->ui_renderer()->Draw(render_info, controller_info);

  // We produce GL errors while rendering. Clear them all so that we can check
  // for errors of subsequent calls.
  while (glGetError() != GL_NO_ERROR) {
  }

// Save screenshot.
#if defined(OS_ANDROID)
  const char screenshot_filename[] = "/sdcard/screenshot.png";
#else
  const char screenshot_filename[] = "screenshot.png";
#endif
  auto bitmap = SaveCurrentFrameBufferToSkBitmap(frame_buffer_size);
  ASSERT_TRUE(bitmap);
  ASSERT_TRUE(SaveSkBitmapToPng(*bitmap, screenshot_filename));

  // Clean up.
  ui = nullptr;
  glDeleteTextures(1, &content_texture);
}

}  // namespace vr
