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
#include "ui/gl/egl_mock.h"
#include "ui/gl/gl_egl_api_implementation.h"
#include "ui/gl/gl_surface_egl.h"

using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::_;

namespace gpu {
namespace gles2 {

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
    gl::SetGLGetProcAddressProc(gl::MockEGLInterface::GetGLProcAddress);
    egl_.reset(new ::testing::NiceMock<::gl::MockEGLInterface>());
    ::gl::MockEGLInterface::SetEGLInterface(egl_.get());

    ON_CALL(*egl_, QueryString(_, EGL_EXTENSIONS))
        .WillByDefault(Return(extensions));
    ON_CALL(*egl_, GetCurrentDisplay())
        .WillByDefault(Return((EGLDisplay)(0x42)));
    ON_CALL(*egl_, GetDisplay(_)).WillByDefault(Return((EGLDisplay)(0x42)));
    ON_CALL(*egl_, Initialize(_, _, _)).WillByDefault(Return(true));
    ON_CALL(*egl_, Terminate(_)).WillByDefault(Return(true));

    gl::ClearBindingsEGL();
    gl::InitializeStaticGLBindingsEGL();
    gl::GLSurfaceEGL::InitializeOneOffForTesting();
  }

  void TeardownMockEGL() { egl_.reset(); }

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
  std::unique_ptr<::testing::NiceMock<::gl::MockEGLInterface>> egl_;
};

TEST_F(GpuFenceManagerTest, Basic) {
  const GLuint kClient1Id = 1;
  const GLuint kClient2Id = 2;

  // Sanity check that our client IDs are invalid at start.
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient1Id));
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient2Id));

  ON_CALL(*egl_, CreateSyncKHR(_, _, _))
      .WillByDefault(Return((EGLSyncKHR)(0x42)));

  // Creating a new fence creates an underlying native sync object.
  EXPECT_CALL(*egl_, CreateSyncKHR(_, EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr))
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
