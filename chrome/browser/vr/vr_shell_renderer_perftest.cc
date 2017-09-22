// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/vr/test/constants.h"
#include "chrome/browser/vr/test/mock_content_input_delegate.h"
#include "chrome/browser/vr/toolbar_state.h"
#include "chrome/browser/vr/ui_browser_interface.h"
#include "chrome/browser/vr/ui_input_manager.h"
#include "chrome/browser/vr/ui_renderer.h"
#include "chrome/browser/vr/ui_scene.h"
#include "chrome/browser/vr/ui_scene_manager.h"
#include "chrome/browser/vr/vr_shell_renderer.h"
#include "components/toolbar/vector_icons.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/test/gl_image_test_template.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace vr {

namespace {

#if defined(OS_ANDROID)
static constexpr char kScreenshotFilename[] = "/sdcard/screenshot.png";
#else
static constexpr char kScreenshotFilename[] = "screenshot.png";
#endif
static constexpr gfx::Size kFrameBufferSize(
    960,
    1080);  // Resolution of Pixel Phone for one eye.
static constexpr gfx::Size kContentTextureSize(256, 256);
static constexpr uint8_t kContentTextureColor[] = {0x0, 0xFF, 0x0,
                                                   0xFF};  // Green.
static const gfx::Transform kViewMatrix;
static constexpr base::TimeTicks kCurrentTime = base::TimeTicks();

static std::unique_ptr<SkBitmap> SaveCurrentFrameBufferToSkBitmap(
    const gfx::Size& size) {
  // Read pixels from frame buffer.
  uint32_t* pixels = new uint32_t[size.width() * size.height()];
  glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE,
               pixels);
  if (glGetError() != GL_NO_ERROR) {
    return nullptr;
  }

  // Flip image vertically since SkBitmap expects the pixels in a different
  // order.
  for (int stride = 0; stride < size.height() / 2; stride++) {
    for (int strideOffset = 0; strideOffset < size.width(); strideOffset++) {
      size_t topPixelIndex = size.width() * stride + strideOffset;
      size_t bottomPixelIndex =
          (size.height() - stride - 1) * size.width() + strideOffset;
      pixels[topPixelIndex] ^= pixels[bottomPixelIndex];
      pixels[bottomPixelIndex] ^= pixels[topPixelIndex];
      pixels[topPixelIndex] ^= pixels[bottomPixelIndex];
    }
  }

  // Create bitmap with pixels.
  std::unique_ptr<SkBitmap> bitmap = base::MakeUnique<SkBitmap>();
  if (!bitmap->installPixels(SkImageInfo::MakeN32(size.width(), size.height(),
                                                  kOpaque_SkAlphaType),
                             pixels, sizeof(uint32_t) * size.width(),
                             [](void* pixels, void* context) {
                               delete[] static_cast<uint32_t*>(pixels);
                             },
                             nullptr)) {
    return nullptr;
  }

  return bitmap;
}

static bool SaveSkBitmapToPng(const SkBitmap& bitmap,
                              const std::string& filename) {
  SkFILEWStream stream(filename.c_str());
  if (!stream.isValid()) {
    return false;
  }
  if (!SkEncodeImage(&stream, bitmap, SkEncodedImageFormat::kPNG, 100)) {
    return false;
  }
  return true;
}

}  // namespace

class MockBrowser : public UiBrowserInterface {
 public:
  MOCK_METHOD0(ExitPresent, void());
  MOCK_METHOD0(ExitFullscreen, void());
  MOCK_METHOD0(NavigateBack, void());
  MOCK_METHOD0(ExitCct, void());
  MOCK_METHOD1(OnUnsupportedMode, void(vr::UiUnsupportedMode mode));
  MOCK_METHOD2(OnExitVrPromptResult,
               void(vr::UiUnsupportedMode reason,
                    vr::ExitVrPromptChoice choice));
  MOCK_METHOD1(OnContentScreenBoundsChanged, void(const gfx::SizeF& bounds));
};

TEST(VrShellRendererPerfTest, RenderUi) {
  // Setup offscreen GL context.
  gl::GLImageTestSupport::InitializeGL();
  scoped_refptr<gl::GLSurface> surface =
      gl::init::CreateOffscreenGLSurface(gfx::Size());
  scoped_refptr<gl::GLContext> context =
      gl::init::CreateGLContext(nullptr, surface.get(), gl::GLContextAttribs());
  context->MakeCurrent(surface.get());

  GLuint vao = 0;
  if (gl::GLContext::GetCurrent()->GetVersionInfo()->IsAtLeastGL(3, 3)) {
    // To avoid glGetVertexAttribiv(0, ...) failing.
    glGenVertexArraysOES(1, &vao);
    glBindVertexArrayOES(vao);
  }

  GLuint framebuffer = gl::GLTestHelper::SetupFramebuffer(
      kFrameBufferSize.width(), kFrameBufferSize.height());
  ASSERT_TRUE(framebuffer);
  glBindFramebufferEXT(GL_FRAMEBUFFER, framebuffer);

  // Create depth buffer.
  GLuint depth_render_buffer = 0;
  glGenRenderbuffersEXT(1, &depth_render_buffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER, depth_render_buffer);
  glRenderbufferStorageEXT(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                           kFrameBufferSize.width(), kFrameBufferSize.height());
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_RENDERBUFFER, depth_render_buffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER, 0);
  ASSERT_EQ(glGetError(), (GLenum)GL_NO_ERROR);

  // Make content texture.
  GLuint content_texture = gl::GLTestHelper::CreateTexture(GL_TEXTURE_2D);
  std::vector<uint8_t> content_pixels(gfx::BufferSizeForBufferFormat(
      kContentTextureSize, gfx::BufferFormat::RGBA_8888));
  gl::GLImageTestSupport::SetBufferDataToColor(
      kContentTextureSize.width(), kContentTextureSize.height(),
      static_cast<int>(gfx::RowSizeForBufferFormat(
          kContentTextureSize.width(), gfx::BufferFormat::RGBA_8888, 0)),
      0, gfx::BufferFormat::RGBA_8888, kContentTextureColor,
      content_pixels.data());
  glBindTexture(GL_TEXTURE_2D, content_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kContentTextureSize.width(),
               kContentTextureSize.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
               content_pixels.data());

  // TODO(tiborg): Make GL_TEXTURE_EXTERNAL_OES texture for content.
  // EGLint eglImageAttributes[] = {EGL_WIDTH,
  //                                kContentTextureSize.width(),
  //                                EGL_HEIGHT,
  //                                kContentTextureSize.height(),
  //                                EGL_MATCH_FORMAT_KHR,
  //                                EGL_FORMAT_RGBA_8888_KHR,
  //                                EGL_IMAGE_PRESERVED_KHR,
  //                                EGL_TRUE,
  //                                EGL_NONE};
  // static_assert(sizeof(uint64_t) == sizeof(EGLClientBuffer), "");
  // uint64_t content_texture_large = content_texture;
  // EGLClientBuffer client_buffer;
  // std::memcpy(&client_buffer, &content_texture_large, sizeof(uint64_t));
  // Fails with SIGSEGV!
  // EGLDisplay display = eglGetCurrentDisplay();
  // EGLImageKHR image =
  //     eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_GL_TEXTURE_2D_KHR,
  //                       client_buffer, eglImageAttributes);

  // GLuint content_texture_egl_external;
  // glGenTextures(1, &content_texture_egl_external);
  // glBindTexture(GL_TEXTURE_2D, content_texture_egl_external);
  // glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

  ASSERT_EQ(glGetError(), (GLenum)GL_NO_ERROR);
  // ASSERT_EQ(eglGetError(), EGL_SUCCESS);

  // Set up scene.
  MockBrowser mock_browser;
  MockContentInputDelegate mock_content_input_delegate;
  ToolbarState toolbar_state(GURL("https://example.com"),
                             security_state::SECURE, &toolbar::kHttpsValidIcon,
                             base::UTF8ToUTF16("Secure"), true, false);

  std::unique_ptr<vr::UiScene> scene(base::MakeUnique<vr::UiScene>());
  std::unique_ptr<vr::UiSceneManager> scene_manager(
      base::MakeUnique<vr::UiSceneManager>(&mock_browser, scene.get(),
                                           &mock_content_input_delegate, false,
                                           false, false));
  bool should_draw_web_vr = false;
  scene_manager->OnGlInitialized(content_texture);
  std::unique_ptr<vr::UiInputManager> input_manager =
      base::MakeUnique<vr::UiInputManager>(scene.get());
  std::unique_ptr<vr::VrShellRenderer> vr_shell_renderer =
      base::MakeUnique<vr::VrShellRenderer>();
  std::unique_ptr<vr::UiRenderer> ui_renderer =
      base::MakeUnique<vr::UiRenderer>(scene.get(), vr_shell_renderer.get());

  scene_manager->SetToolbarState(toolbar_state);

  // Set up render info.
  ControllerInfo controller_info;
  controller_info.target_point = {0, 0, -1};
  controller_info.target_point = {0, 0, 0};
  controller_info.transform = kViewMatrix;
  controller_info.opacity = 1.0f;
  controller_info.reticle_render_target = nullptr;

  RenderInfo render_info;
  render_info.head_pose = kViewMatrix;
  render_info.surface_texture_size = kFrameBufferSize;
  render_info.left_eye_info.viewport = {0, 0, kFrameBufferSize.width(),
                                        kFrameBufferSize.height()};
  render_info.left_eye_info.view_matrix = kViewMatrix;

  render_info.left_eye_info.proj_matrix = kProjMatrix;
  render_info.left_eye_info.view_proj_matrix = kProjMatrix * kViewMatrix;
  render_info.right_eye_info = render_info.left_eye_info;
  render_info.right_eye_info.viewport = {0, 0, 0, 0};

  // Render UI.
  scene->OnBeginFrame(kCurrentTime, gfx::Vector3dF(0.f, 0.f, -1.0f));
  scene->PrepareToDraw();
  ui_renderer->Draw(render_info, controller_info, should_draw_web_vr);
  if (!scene->GetViewportAwareElements().empty() && !should_draw_web_vr) {
    ui_renderer->DrawViewportAware(render_info, controller_info, false);
  }

  // Clear all GL errors.
  while (glGetError() != GL_NO_ERROR) {
    glGetError();
  }

  // Save screenshot.
  auto bitmap = SaveCurrentFrameBufferToSkBitmap(kFrameBufferSize);
  ASSERT_TRUE(bitmap);
  ASSERT_TRUE(SaveSkBitmapToPng(*bitmap, kScreenshotFilename));

  // Clean up.
  glDeleteTextures(1, &content_texture);
  glDeleteFramebuffersEXT(1, &framebuffer);
  if (vao) {
    glDeleteVertexArraysOES(1, &vao);
  }
  context->ReleaseCurrent(surface.get());
  context = nullptr;
  surface = nullptr;
  gl::GLImageTestSupport::CleanupGL();
}

}  // namespace vr
