// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/raster_implementation_gles.h"

#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface_stub.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Return;
using testing::SetArgPointee;

namespace gpu {
namespace raster {

class RasterMockGLES2Interface : public gles2::GLES2InterfaceStub {
 public:
  // Command buffer Flush / Finish.
  MOCK_METHOD0(Finish, void());
  MOCK_METHOD0(ShallowFlushCHROMIUM, void());
  MOCK_METHOD0(OrderingBarrierCHROMIUM, void());

  // SyncTokens.
  MOCK_METHOD0(InsertFenceSyncCHROMIUM, GLuint64());
  MOCK_METHOD2(GenSyncTokenCHROMIUM,
               void(GLuint64 fence_sync, GLbyte* sync_token));
  MOCK_METHOD2(GenUnverifiedSyncTokenCHROMIUM,
               void(GLuint64 fence_sync, GLbyte* sync_token));
  MOCK_METHOD2(VerifySyncTokensCHROMIUM,
               void(GLbyte** sync_tokens, GLsizei count));
  MOCK_METHOD1(WaitSyncTokenCHROMIUM, void(const GLbyte* sync_token));

  // Texture objects.
  MOCK_METHOD2(GenTextures, void(GLsizei n, GLuint* textures));

  // Mailboxes.
  MOCK_METHOD1(GenMailboxCHROMIUM, void(GLbyte*));
  MOCK_METHOD2(ProduceTextureDirectCHROMIUM,
               void(GLuint texture, const GLbyte* mailbox));
  MOCK_METHOD1(CreateAndConsumeTextureCHROMIUM, GLuint(const GLbyte* mailbox));
};

class RasterImplementationGLESTest : public testing::Test {
 protected:
  RasterImplementationGLESTest() {}

  void SetUp() override {
    gpu::Capabilities capabilities;
    gl_.reset(new RasterMockGLES2Interface());
    ri_.reset(new RasterImplementationGLES(gl_.get(), capabilities));
  }

  void TearDown() override {}

  std::unique_ptr<RasterMockGLES2Interface> gl_;
  std::unique_ptr<RasterImplementationGLES> ri_;
};

TEST_F(RasterImplementationGLESTest, Finish) {
  EXPECT_CALL(*gl_, Finish()).Times(1);
  ri_->Finish();
}

TEST_F(RasterImplementationGLESTest, ShallowFlushCHROMIUM) {
  EXPECT_CALL(*gl_, ShallowFlushCHROMIUM()).Times(1);
  ri_->ShallowFlushCHROMIUM();
}

TEST_F(RasterImplementationGLESTest, OrderingBarrierCHROMIUM) {
  EXPECT_CALL(*gl_, OrderingBarrierCHROMIUM()).Times(1);
  ri_->OrderingBarrierCHROMIUM();
}

TEST_F(RasterImplementationGLESTest, InsertFenceSyncCHROMIUM) {
  const GLuint64 kFenceSync = 0x123u;
  GLuint64 fence_sync = 0;

  EXPECT_CALL(*gl_, InsertFenceSyncCHROMIUM()).WillOnce(Return(kFenceSync));
  fence_sync = ri_->InsertFenceSyncCHROMIUM();
  EXPECT_EQ(kFenceSync, fence_sync);
}

TEST_F(RasterImplementationGLESTest, GenSyncTokenCHROMIUM) {
  const GLuint64 kFenceSync = 0x123u;
  GLbyte sync_token_data[GL_SYNC_TOKEN_SIZE_CHROMIUM] = {};

  EXPECT_CALL(*gl_, GenSyncTokenCHROMIUM(kFenceSync, sync_token_data)).Times(1);
  ri_->GenSyncTokenCHROMIUM(kFenceSync, sync_token_data);
}

TEST_F(RasterImplementationGLESTest, GenUnverifiedSyncTokenCHROMIUM) {
  const GLuint64 kFenceSync = 0x123u;
  GLbyte sync_token_data[GL_SYNC_TOKEN_SIZE_CHROMIUM] = {};

  EXPECT_CALL(*gl_, GenUnverifiedSyncTokenCHROMIUM(kFenceSync, sync_token_data))
      .Times(1);
  ri_->GenUnverifiedSyncTokenCHROMIUM(kFenceSync, sync_token_data);
}

TEST_F(RasterImplementationGLESTest, VerifySyncTokensCHROMIUM) {
  const GLsizei kNumSyncTokens = 2;
  GLbyte sync_token_data[GL_SYNC_TOKEN_SIZE_CHROMIUM][kNumSyncTokens] = {};
  GLbyte* sync_tokens[2] = {sync_token_data[0], sync_token_data[1]};

  EXPECT_CALL(*gl_, VerifySyncTokensCHROMIUM(sync_tokens, kNumSyncTokens))
      .Times(1);
  ri_->VerifySyncTokensCHROMIUM(sync_tokens, kNumSyncTokens);
}

TEST_F(RasterImplementationGLESTest, WaitSyncTokenCHROMIUM) {
  GLbyte sync_token_data[GL_SYNC_TOKEN_SIZE_CHROMIUM] = {};

  EXPECT_CALL(*gl_, WaitSyncTokenCHROMIUM(sync_token_data)).Times(1);
  ri_->WaitSyncTokenCHROMIUM(sync_token_data);
}

TEST_F(RasterImplementationGLESTest, GenMailboxCHROMIUM) {
  gpu::Mailbox mailbox;
  EXPECT_CALL(*gl_, GenMailboxCHROMIUM(mailbox.name)).Times(1);
  ri_->GenMailboxCHROMIUM(mailbox.name);
}

TEST_F(RasterImplementationGLESTest, ProduceTextureDirectCHROMIUM) {
  const GLuint kTextureId = 23;
  GLuint texture_id = 0;
  gpu::Mailbox mailbox;

  EXPECT_CALL(*gl_, GenTextures(1, &texture_id))
      .WillOnce(SetArgPointee<1>(kTextureId));
  EXPECT_CALL(*gl_, GenMailboxCHROMIUM(mailbox.name)).Times(1);
  EXPECT_CALL(*gl_, ProduceTextureDirectCHROMIUM(kTextureId, mailbox.name))
      .Times(1);

  ri_->GenTextures(1, &texture_id);
  ri_->GenMailboxCHROMIUM(mailbox.name);
  ri_->ProduceTextureDirectCHROMIUM(texture_id, mailbox.name);
}

TEST_F(RasterImplementationGLESTest, CreateAndConsumeTextureCHROMIUM) {
  const GLuint kTextureId = 23;
  GLuint texture_id = 0;
  gpu::Mailbox mailbox;

  EXPECT_CALL(*gl_, CreateAndConsumeTextureCHROMIUM(mailbox.name))
      .WillOnce(Return(kTextureId));
  texture_id = ri_->CreateAndConsumeTextureCHROMIUM(mailbox.name);
  EXPECT_EQ(kTextureId, texture_id);
}

}  // namespace raster
}  // namespace gpu
