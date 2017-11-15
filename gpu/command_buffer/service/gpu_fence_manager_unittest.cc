// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gpu_fence_manager.h"

#include "gpu/command_buffer/client/client_test_helper.h"
#include "gpu/command_buffer/service/error_state_mock.h"
#include "gpu/command_buffer/service/feature_info.h"
#include "gpu/command_buffer/service/gpu_service_test.h"
#include "gpu/command_buffer/service/test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_egl_api_implementation.h"
#include "ui/gl/gl_fence_egl.h"
#include "ui/gl/gl_surface_egl.h"

using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::_;

namespace gpu {
namespace gles2 {

class MockEGLApi : public gl::RealEGLApi {
 public:
  MOCK_METHOD0(eglGetCurrentDisplayFn, EGLDisplay());
  MOCK_METHOD1(eglGetDisplayFn,
               EGLDisplay(EGLNativeDisplayType native_display));
  MOCK_METHOD3(eglCreateSyncKHRFn,
               EGLSyncKHR(EGLDisplay display,
                          EGLenum type,
                          const EGLint* attrs));
  MOCK_METHOD3(eglWaitSyncKHRFn,
               EGLint(EGLDisplay display, EGLSyncKHR fence, EGLint flags));
  MOCK_METHOD2(eglDestroySyncKHRFn,
               EGLBoolean(EGLDisplay display, EGLSyncKHR fence));
  MOCK_METHOD2(eglQueryStringFn, const char*(EGLDisplay display, EGLint name));
  MOCK_METHOD3(eglInitializeFn,
               EGLBoolean(EGLDisplay display, EGLint* major, EGLint* minor));
  MOCK_METHOD1(eglTerminateFn, EGLBoolean(EGLDisplay display));
};

class GpuFenceManagerTest : public GpuServiceTest {
 public:
  GpuFenceManagerTest() {
    GpuDriverBugWorkarounds gpu_driver_bug_workaround;
    feature_info_ = new FeatureInfo(gpu_driver_bug_workaround);
  }

  ~GpuFenceManagerTest() override {}

 protected:
  void SetUp() override {
    GpuServiceTest::SetUp();
    SetupMockEGL("EGL_ANDROID_native_fence_sync");
    SetupFeatureInfo("", "OpenGL ES 2.0", CONTEXT_TYPE_OPENGLES2);
    error_state_.reset(new ::testing::StrictMock<MockErrorState>());
    manager_.reset(new GpuFenceManager());
  }

  void TearDown() override {
    manager_->Destroy(false);
    manager_.reset();
    GpuServiceTest::TearDown();
    TeardownMockEGL();
  }

  void SetupMockEGL(const char* extensions) {
    // FIXME: clean this up and/or put in a helper class.
    old_egl_ = gl::g_current_egl_context;
    egl_ = base::MakeUnique<MockEGLApi>();
    ON_CALL(*egl_, eglQueryStringFn(_, EGL_EXTENSIONS))
        .WillByDefault(Return(extensions));
    ON_CALL(*egl_, eglGetCurrentDisplayFn())
        .WillByDefault(Return((EGLDisplay)(0x42)));
    ON_CALL(*egl_, eglGetDisplayFn(_))
        .WillByDefault(Return((EGLDisplay)(0x42)));
    ON_CALL(*egl_, eglInitializeFn(_, _, _)).WillByDefault(Return(true));
    ON_CALL(*egl_, eglTerminateFn(_)).WillByDefault(Return(true));
    gl::g_current_egl_context = egl_.get();
    gl::GLSurfaceEGL::InitializeOneOff(nullptr);
  }

  void TeardownMockEGL() {
    gl::g_current_egl_context = old_egl_;
    egl_.reset();
  }

  void SetupFeatureInfo(const char* gl_extensions,
                        const char* gl_version,
                        ContextType context_type) {
    TestHelper::SetupFeatureInfoInitExpectationsWithGLVersion(
        gl_.get(), gl_extensions, "", gl_version, context_type);
    feature_info_->InitializeForTesting(context_type);
    ASSERT_TRUE(feature_info_->context_type() == context_type);
    if (feature_info_->IsWebGL2OrES3Context()) {
      EXPECT_CALL(*gl_, GetIntegerv(GL_MAX_COLOR_ATTACHMENTS, _))
          .WillOnce(SetArgPointee<1>(8))
          .RetiresOnSaturation();
      EXPECT_CALL(*gl_, GetIntegerv(GL_MAX_DRAW_BUFFERS, _))
          .WillOnce(SetArgPointee<1>(8))
          .RetiresOnSaturation();
      feature_info_->EnableES3Validators();
    }
  }

  scoped_refptr<FeatureInfo> feature_info_;
  std::unique_ptr<GpuFenceManager> manager_;
  std::unique_ptr<MockErrorState> error_state_;
  gl::EGLApi* old_egl_;
  std::unique_ptr<MockEGLApi> egl_;
};

TEST_F(GpuFenceManagerTest, Basic) {
  const GLuint kClient1Id = 1;
  const GLuint kClient2Id = 2;

  // Sanity check that our client ID is invalid at start.
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient1Id));

  ON_CALL(*egl_, eglCreateSyncKHRFn(_, _, _))
      .WillByDefault(Return((EGLSyncKHR)(0x42)));

  // Creating a new fence creates an underlying native sync object.
  EXPECT_CALL(*egl_,
              eglCreateSyncKHRFn(_, EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr))
      .Times(1)
      .RetiresOnSaturation();
  EXPECT_CALL(*gl_, Flush()).Times(1).RetiresOnSaturation();
  manager_->CreateNewGpuFence(kClient1Id);
  EXPECT_TRUE(manager_->IsValidGpuFence(kClient1Id));

  // Removing the fence marks it invalid.
  manager_->RemoveGpuFence(kClient1Id);
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient1Id));

  // Removing a non-existent fence does not crash.
  manager_->RemoveGpuFence(kClient2Id);
}

}  // namespace gles2
}  // namespace gpu
