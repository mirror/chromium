// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gpu_fence_manager.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "gpu/command_buffer/service/error_state_mock.h"
#include "gpu/command_buffer/service/feature_info.h"
#include "gpu/command_buffer/service/gpu_service_test.h"
#include "gpu/command_buffer/service/test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/gpu_fence_handle.h"
#include "ui/gl/egl_mock.h"
#include "ui/gl/gl_egl_api_implementation.h"
#include "ui/gl/gl_surface_egl.h"

#include <unistd.h>

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
    SetupMockEGL("EGL_ANDROID_native_fence_sync EGL_KHR_wait_sync");
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

    const EGLDisplay kDummyDisplay = reinterpret_cast<EGLDisplay>(0x1001);
    ON_CALL(*egl_, QueryString(_, EGL_EXTENSIONS))
        .WillByDefault(Return(extensions));
    ON_CALL(*egl_, GetCurrentDisplay()).WillByDefault(Return(kDummyDisplay));
    ON_CALL(*egl_, GetDisplay(_)).WillByDefault(Return(kDummyDisplay));
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
  }

  scoped_refptr<FeatureInfo> feature_info_;
  std::unique_ptr<GpuFenceManager> manager_;
  std::unique_ptr<MockErrorState> error_state_;
  std::unique_ptr<::testing::NiceMock<::gl::MockEGLInterface>> egl_;
};

TEST_F(GpuFenceManagerTest, Basic) {
  const GLuint kClient1Id = 1;
  const GLuint kClient2Id = 2;
  const EGLSyncKHR kDummySync = reinterpret_cast<EGLSyncKHR>(0x2001);
  const EGLint kFenceFD = dup(1);

  // Sanity check that our client IDs are invalid at start.
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient1Id));
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient2Id));

  // Creating a new fence creates an underlying native sync object.
  EXPECT_CALL(*egl_, CreateSyncKHR(_, EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr))
      .Times(1)
      .WillOnce(Return(kDummySync))
      .RetiresOnSaturation();
  EXPECT_CALL(*gl_, Flush()).Times(1).RetiresOnSaturation();
  // We use a dup'ed STDOUT as the file descriptor for testing. Since
  // it will be closed on GpuFence destruction, only return it one time.
  EXPECT_CALL(*egl_, DupNativeFenceFDANDROID(_, kDummySync))
      .Times(1)
      .WillOnce(Return(kFenceFD))
      .RetiresOnSaturation();
  EXPECT_TRUE(manager_->CreateNewGpuFence(kClient1Id));
  EXPECT_TRUE(manager_->IsValidGpuFence(kClient1Id));

  // Try a server wait on it.
  EXPECT_CALL(*egl_, WaitSyncKHR(_, kDummySync, _))
      .Times(1)
      .WillOnce(Return(EGL_TRUE))
      .RetiresOnSaturation();
  EXPECT_TRUE(manager_->WaitGpuFence(kClient1Id));

  // Get its GpuFenceHandle.
  gfx::GpuFenceHandle handle = manager_->GetGpuFenceHandle(kClient1Id);
  EXPECT_EQ(handle.type, gfx::GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC);
  EXPECT_EQ(handle.native_fd.fd, kFenceFD);

  // Removing the fence marks it invalid.
  EXPECT_CALL(*egl_, DestroySyncKHR(_, kDummySync))
      .Times(1)
      .RetiresOnSaturation();
  EXPECT_TRUE(manager_->RemoveGpuFence(kClient1Id));
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient1Id));

  // Removing a non-existent fence does not crash.
  EXPECT_FALSE(manager_->RemoveGpuFence(kClient2Id));
}

TEST_F(GpuFenceManagerTest, Duplication) {
  const GLuint kClient1Id = 1;
  const EGLSyncKHR kDummySync = reinterpret_cast<EGLSyncKHR>(0x2001);
  const EGLint kFenceFD = dup(1);

  // Sanity check that our client IDs are invalid at start.
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient1Id));

  // Create a handle.
  gfx::GpuFenceHandle handle = manager_->GetGpuFenceHandle(kClient1Id);
  handle.type = gfx::GpuFenceHandleType::ANDROID_NATIVE_FENCE_SYNC;
  handle.native_fd = base::FileDescriptor(kFenceFD, true);

  // Create a duplicate fence object from it.
  EXPECT_CALL(*egl_, CreateSyncKHR(_, EGL_SYNC_NATIVE_FENCE_ANDROID, _))
      .Times(1)
      .WillOnce(Return(kDummySync))
      .RetiresOnSaturation();
  EXPECT_CALL(*gl_, Flush()).Times(1).RetiresOnSaturation();
  EXPECT_TRUE(manager_->CreateDuplicateGpuFence(kClient1Id, handle));
  EXPECT_TRUE(manager_->IsValidGpuFence(kClient1Id));

  // Try a server wait on it.
  EXPECT_CALL(*egl_, WaitSyncKHR(_, kDummySync, _))
      .Times(1)
      .WillOnce(Return(EGL_TRUE))
      .RetiresOnSaturation();
  EXPECT_TRUE(manager_->WaitGpuFence(kClient1Id));

  // Cleanup.
  EXPECT_TRUE(manager_->RemoveGpuFence(kClient1Id));
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient1Id));
}

TEST_F(GpuFenceManagerTest, Destruction) {
  const GLuint kClient1Id = 1;
  const EGLSyncKHR kDummySync = reinterpret_cast<EGLSyncKHR>(0x2001);

  // Sanity check that our client IDs are invalid at start.
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient1Id));

  // Create a fence object.
  EXPECT_CALL(*egl_, CreateSyncKHR(_, _, _)).WillOnce(Return(kDummySync));
  EXPECT_CALL(*gl_, Flush()).Times(1).RetiresOnSaturation();
  EXPECT_TRUE(manager_->CreateNewGpuFence(kClient1Id));
  EXPECT_TRUE(manager_->IsValidGpuFence(kClient1Id));

  // Destroying the manager destroys any pending sync objects.
  EXPECT_CALL(*egl_, DestroySyncKHR(_, kDummySync))
      .Times(1)
      .RetiresOnSaturation();
  manager_->Destroy(true);
}

TEST_F(GpuFenceManagerTest, CallbackFirst) {
  const GLuint kClient1Id = 1;
  bool callback_was_run = false;

  // Sanity check that our client IDs are invalid at start.
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient1Id));

  // Attach a callback to this not-yet-existing GpuFence entry,
  // this should fail.
  bool success = manager_->AddCallback(
      kClient1Id, base::BindOnce([](bool* out) { *out = true; },
                                 base::Unretained(&callback_was_run)));
  EXPECT_FALSE(success);
}

TEST_F(GpuFenceManagerTest, CallbackLater) {
  const GLuint kClient2Id = 2;
  const EGLSyncKHR kDummySync = reinterpret_cast<EGLSyncKHR>(0x2001);
  const EGLint kFenceFD = dup(1);

  bool callback_was_run = false;

  // Sanity check that our client IDs are invalid at start.
  EXPECT_FALSE(manager_->IsValidGpuFence(kClient2Id));

  // Create the fence object first.
  EXPECT_CALL(*egl_, DupNativeFenceFDANDROID(_, kDummySync))
      .Times(1)
      .WillOnce(Return(kFenceFD))
      .RetiresOnSaturation();
  EXPECT_CALL(*egl_, CreateSyncKHR(_, _, _)).WillOnce(Return(kDummySync));
  EXPECT_CALL(*gl_, Flush()).Times(1).RetiresOnSaturation();
  EXPECT_TRUE(manager_->CreateNewGpuFence(kClient2Id));
  EXPECT_TRUE(manager_->IsValidGpuFence(kClient2Id));
  EXPECT_FALSE(callback_was_run);

  // Attach a callback to this existing GpuFence entry.
  // Callback should run immediately.
  bool success = manager_->AddCallback(
      kClient2Id, base::BindOnce([](bool* out) { *out = true; },
                                 base::Unretained(&callback_was_run)));
  EXPECT_TRUE(success);
  EXPECT_TRUE(callback_was_run);
}

}  // namespace gles2
}  // namespace gpu
