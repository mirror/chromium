// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/service_discardable_manager.h"

#include "gpu/command_buffer/client/client_test_helper.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder_mock.h"
#include "gpu/command_buffer/service/gpu_service_test.h"
#include "gpu/command_buffer/service/image_manager.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "gpu/command_buffer/service/mocks.h"
#include "gpu/command_buffer/service/test_helper.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_image_stub.h"
#include "ui/gl/gl_mock.h"
#include "ui/gl/gl_switches.h"

using ::testing::Pointee;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::InSequence;

namespace gpu {
namespace gles2 {
namespace {

void CreateLockedHandlesForTesting(
    std::unique_ptr<ServiceDiscardableHandle>* service_handle,
    std::unique_ptr<ClientDiscardableHandle>* client_handle) {
  std::unique_ptr<base::SharedMemory> shared_mem(new base::SharedMemory);
  shared_mem->CreateAndMapAnonymous(sizeof(uint32_t));
  scoped_refptr<gpu::Buffer> buffer =
      MakeBufferFromSharedMemory(std::move(shared_mem), sizeof(uint32_t));

  client_handle->reset(new ClientDiscardableHandle(buffer, 0, 0));
  service_handle->reset(new ServiceDiscardableHandle(buffer, 0, 0));
}

ServiceDiscardableHandle CreateLockedServiceHandleForTesting() {
  std::unique_ptr<ServiceDiscardableHandle> service_handle;
  std::unique_ptr<ClientDiscardableHandle> client_handle;
  CreateLockedHandlesForTesting(&service_handle, &client_handle);
  return *service_handle;
}

class MockDestructionObserver : public TextureManager::DestructionObserver {
 public:
  MOCK_METHOD1(OnTextureManagerDestroying, void(TextureManager* manager));
  MOCK_METHOD1(OnTextureRefDestroying, void(TextureRef* ref));
};

// A small texture that should never run up against our limits.
static const uint32_t kSmallTextureDim = 16;
static const size_t kSmallTextureSize = 4 * kSmallTextureDim * kSmallTextureDim;

}  // namespace

class ServiceDiscardableManagerTest : public GpuServiceTest {
 public:
  ServiceDiscardableManagerTest() {}
  ~ServiceDiscardableManagerTest() override {}

 protected:
  void SetUp() override {
    GpuServiceTest::SetUp();
    decoder_.reset(new MockGLES2Decoder(&command_buffer_service_));
    feature_info_ = new FeatureInfo();
    InitNewContextGroup();
  }

  void TearDown() override {
    for (auto& set : sets_) {
    EXPECT_CALL(set->destruction_observer, OnTextureManagerDestroying(_))
        .RetiresOnSaturation();
    // Texture manager will destroy the 6 black/default textures.
    EXPECT_CALL(*gl_, DeleteTextures(TextureManager::kNumDefaultTextures, _));

    set->context_group->Destroy(decoder_.get(), true);
    set->context_group = nullptr;
    }
    sets_.clear();

    EXPECT_EQ(0u, discardable_manager_.NumCacheEntriesForTesting());
    GpuServiceTest::TearDown();
  }

  void InitNewContextGroup() {
    std::unique_ptr<ContextGroupSet> cgs(new ContextGroupSet());
    cgs->context_group = scoped_refptr<ContextGroup>(
        new ContextGroup(gpu_preferences_, nullptr, nullptr, nullptr, nullptr,
                         feature_info_, false, &image_manager_, nullptr,
                         nullptr, GpuFeatureInfo(), &discardable_manager_));
    TestHelper::SetupContextGroupInitExpectations(
        gl_.get(), DisallowedFeatures(), "", "", CONTEXT_TYPE_OPENGLES2, false);
    cgs->context_group->Initialize(decoder_.get(), CONTEXT_TYPE_OPENGLES2,
                               DisallowedFeatures());
    cgs->texture_manager = cgs->context_group->texture_manager();
    cgs->texture_manager->AddObserver(&cgs->destruction_observer);

    sets_.push_back(std::move(cgs));
  }

  gles2::TextureManager* TextureManager(int set_id) { return sets_[set_id]->texture_manager; }
  MockDestructionObserver* DestructionObserver(int set_id) { return &sets_[set_id]->destruction_observer; }

  void ExpectUnlockedTextureDeletion(int set_id, uint32_t client_id) {
    TextureRef* ref = discardable_manager_.UnlockedTextureRefForTesting(
        client_id, sets_[set_id]->texture_manager);
    ExpectTextureRefDeletion(set_id, ref);
  }

  void ExpectTextureDeletion(int set_id, uint32_t client_id) {
    TextureRef* ref = sets_[set_id]->texture_manager->GetTexture(client_id);
    ExpectTextureRefDeletion(set_id, ref);
  }

  void ExpectTextureRefDeletion(int set_id, TextureRef* ref) {
    EXPECT_NE(nullptr, ref);
    ref->AddObserver();
    EXPECT_CALL(sets_[set_id]->destruction_observer, OnTextureRefDestroying(ref))
        .WillOnce(Invoke([](TextureRef* ref) { ref->RemoveObserver(); }));
    EXPECT_CALL(*gl_, DeleteTextures(1, Pointee(ref->service_id())))
        .RetiresOnSaturation();
  }

  // Versions of the above for the default, 1 set, case.
  gles2::TextureManager* TextureManager() { return TextureManager(0); }
  MockDestructionObserver* DestructionObserver() { return DestructionObserver(0); }
  void ExpectUnlockedTextureDeletion(uint32_t client_id) { ExpectUnlockedTextureDeletion(0, client_id); }
  void ExpectTextureDeletion(uint32_t client_id) { ExpectTextureDeletion(0, client_id); }

  gles2::ImageManager image_manager_;
  ServiceDiscardableManager discardable_manager_;
  GpuPreferences gpu_preferences_;
  scoped_refptr<FeatureInfo> feature_info_;
  FakeCommandBufferServiceBase command_buffer_service_;
  std::unique_ptr<MockGLES2Decoder> decoder_;
  
  struct ContextGroupSet {
    MockDestructionObserver destruction_observer;
    gles2::TextureManager* texture_manager;
    scoped_refptr<gles2::ContextGroup> context_group;
  };

  std::vector<std::unique_ptr<ContextGroupSet>> sets_;  
};

TEST_F(ServiceDiscardableManagerTest, BasicUsage) {
  const GLuint kClientId = 1;
  const GLuint kServiceId = 2;

  // Create and insert a new texture.
  TextureManager()->CreateTexture(kClientId, kServiceId);
  auto handle = CreateLockedServiceHandleForTesting();
  discardable_manager_.InsertLockedTexture(kClientId, kSmallTextureSize,
                                           TextureManager(), handle);
  EXPECT_EQ(1u, discardable_manager_.NumCacheEntriesForTesting());
  EXPECT_TRUE(discardable_manager_.IsEntryLockedForTesting(kClientId,
                                                           TextureManager()));
  EXPECT_NE(nullptr, TextureManager()->GetTexture(kClientId));

  // Unlock the texture, ServiceDiscardableManager should take ownership of the
  // TextureRef.
  gles2::TextureRef* texture_to_unbind;
  EXPECT_TRUE(discardable_manager_.UnlockTexture(kClientId, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_FALSE(discardable_manager_.IsEntryLockedForTesting(kClientId,
                                                            TextureManager()));
  EXPECT_EQ(nullptr, TextureManager()->GetTexture(kClientId));

  // Re-lock the texture, the TextureManager should now resume ownership of
  // the TextureRef.
  discardable_manager_.LockTexture(kClientId, TextureManager());
  EXPECT_NE(nullptr, TextureManager()->GetTexture(kClientId));

  // Delete the texture from the TextureManager, it should also be removed from
  // the ServiceDiscardableManager.
  ExpectTextureDeletion(kClientId);
  TextureManager()->RemoveTexture(kClientId);
  EXPECT_EQ(0u, discardable_manager_.NumCacheEntriesForTesting());
}

TEST_F(ServiceDiscardableManagerTest, DeleteAtShutdown) {
  // Create 8 small textures (which will not hit memory limits), leaving every
  // other one unlocked.
  for (int i = 1; i <= 8; ++i) {
    TextureManager()->CreateTexture(i, i);
    auto handle = CreateLockedServiceHandleForTesting();
    discardable_manager_.InsertLockedTexture(i, kSmallTextureSize,
                                             TextureManager(), handle);
    if (i % 2) {
      TextureRef* texture_to_unbind;
      EXPECT_TRUE(discardable_manager_.UnlockTexture(i, TextureManager(),
                                                     &texture_to_unbind));
      EXPECT_NE(nullptr, texture_to_unbind);
    }
  }

  // Expect that all 8 will be deleted at shutdown, regardless of
  // locked/unlocked state.
  for (int i = 1; i <= 8; ++i) {
    if (i % 2) {
      ExpectUnlockedTextureDeletion(i);
    } else {
      ExpectTextureDeletion(i);
    }
  }

  // Let the test shut down, the expectations should be fulfilled.
}

TEST_F(ServiceDiscardableManagerTest, UnlockInvalid) {
  const GLuint kClientId = 1;
  gles2::TextureRef* texture_to_unbind;
  EXPECT_FALSE(discardable_manager_.UnlockTexture(kClientId, TextureManager(),
                                                  &texture_to_unbind));
  EXPECT_EQ(nullptr, texture_to_unbind);
}

TEST_F(ServiceDiscardableManagerTest, LimitsSimple) {
  const size_t fixed_cache_size = 4 * 1024 * 1024;
  discardable_manager_.SetLimitsForTesting(fixed_cache_size, fixed_cache_size, 0);

  // Size textures so that four fit in the discardable manager.
  const size_t texture_size = fixed_cache_size / 4;
  const size_t large_texture_size = 3 * texture_size;

  // Create 4 textures, this should fill up the discardable cache.
  for (int i = 1; i < 5; ++i) {
    TextureManager()->CreateTexture(i, i);
    auto handle = CreateLockedServiceHandleForTesting();
    discardable_manager_.InsertLockedTexture(i, texture_size, TextureManager(),
                                             handle);
  }

  gles2::TextureRef* texture_to_unbind;
  EXPECT_TRUE(discardable_manager_.UnlockTexture(3, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_TRUE(discardable_manager_.UnlockTexture(1, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_TRUE(discardable_manager_.UnlockTexture(2, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_TRUE(discardable_manager_.UnlockTexture(4, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);

  // Allocate four more textures - the previous 4 should be evicted / deleted in
  // LRU order.
  {
    InSequence s;
    ExpectUnlockedTextureDeletion(3);
    ExpectUnlockedTextureDeletion(1);
    ExpectUnlockedTextureDeletion(2);
    ExpectUnlockedTextureDeletion(4);
  }

  for (int i = 5; i < 9; ++i) {
    TextureManager()->CreateTexture(i, i);
    auto handle = CreateLockedServiceHandleForTesting();
    discardable_manager_.InsertLockedTexture(i, texture_size, TextureManager(),
                                             handle);
  }

  // Ensure that the above expectations are handled by this point.
  Mock::VerifyAndClearExpectations(gl_.get());
  Mock::VerifyAndClearExpectations(DestructionObserver());

  // Unlock the next four textures:
  EXPECT_TRUE(discardable_manager_.UnlockTexture(5, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_TRUE(discardable_manager_.UnlockTexture(6, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_TRUE(discardable_manager_.UnlockTexture(8, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_TRUE(discardable_manager_.UnlockTexture(7, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);

  // Allocate one more *large* texture, it should evict the LRU 3 textures.
  {
    InSequence s;
    ExpectUnlockedTextureDeletion(5);
    ExpectUnlockedTextureDeletion(6);
    ExpectUnlockedTextureDeletion(8);
  }

  TextureManager()->CreateTexture(9, 9);
  auto handle = CreateLockedServiceHandleForTesting();
  discardable_manager_.InsertLockedTexture(9, large_texture_size,
                                           TextureManager(), handle);

  // Expect the two remaining textures to clean up.
  ExpectTextureDeletion(9);
  ExpectUnlockedTextureDeletion(7);
}

TEST_F(ServiceDiscardableManagerTest, LimitsGrowth) {
  const size_t min_cache_size = 1 * 1024 * 1024;
  const size_t max_cache_size = 2 * 1024 * 1024;
  const size_t cache_size_growth = min_cache_size;
  discardable_manager_.SetLimitsForTesting(min_cache_size, max_cache_size, cache_size_growth);

  // Init a second and third context group for this test.
  InitNewContextGroup();
  InitNewContextGroup();

  EXPECT_EQ(min_cache_size, discardable_manager_.GetLimitForTesting());

  // Size textures so that exactly one fits in our min cache.
  const size_t texture_size = min_cache_size;
  TextureManager()->CreateTexture(1, 1);
  auto handle = CreateLockedServiceHandleForTesting();
  discardable_manager_.InsertLockedTexture(1, texture_size, TextureManager(), handle);

  // Cache should still have min cache size.
  EXPECT_EQ(min_cache_size, discardable_manager_.GetLimitForTesting());

  // Unlock the texture. It should remain in the cache.
  gles2::TextureRef* texture_to_unbind;
  EXPECT_TRUE(discardable_manager_.UnlockTexture(1, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_EQ(1u, discardable_manager_.NumCacheEntriesForTesting());
  EXPECT_EQ(min_cache_size, discardable_manager_.TotalSizeForTesting());

  // Add a second texture from the same manager, it should exceed the cache evicting the first one.
  ExpectUnlockedTextureDeletion(1);
  TextureManager()->CreateTexture(2, 2);
  auto handle2 = CreateLockedServiceHandleForTesting();
  discardable_manager_.InsertLockedTexture(2, texture_size, TextureManager(), handle2);
  EXPECT_EQ(1u, discardable_manager_.NumCacheEntriesForTesting());
  EXPECT_EQ(min_cache_size, discardable_manager_.TotalSizeForTesting());

  // Unlock the texture. It should remain in the cache.
  EXPECT_TRUE(discardable_manager_.UnlockTexture(2, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_EQ(1u, discardable_manager_.NumCacheEntriesForTesting());
  EXPECT_EQ(min_cache_size, discardable_manager_.TotalSizeForTesting());

  // Add a third texture from a different manager. it should increase our cache limit and lead to no
  // evictions.
  TextureManager(1)->CreateTexture(3, 3);
  auto handle3 = CreateLockedServiceHandleForTesting();
  discardable_manager_.InsertLockedTexture(3, texture_size, TextureManager(1), handle3);
  EXPECT_EQ(2u, discardable_manager_.NumCacheEntriesForTesting());
  EXPECT_EQ(max_cache_size, discardable_manager_.TotalSizeForTesting());

  ExpectTextureDeletion(3, 1);
  ExpectUnlockedTextureDeletion(2);

  // // Create 4 textures, this should fill up the discardable cache.
  // for (int i = 1; i < 5; ++i) {
  //   TextureManager()->CreateTexture(i, i);
  //   auto handle = CreateLockedServiceHandleForTesting();
  //   discardable_manager_.InsertLockedTexture(i, texture_size, TextureManager(),
  //                                            handle);
  // }

  // gles2::TextureRef* texture_to_unbind;
  // EXPECT_TRUE(discardable_manager_.UnlockTexture(3, TextureManager(),
  //                                                &texture_to_unbind));
  // EXPECT_NE(nullptr, texture_to_unbind);
  // EXPECT_TRUE(discardable_manager_.UnlockTexture(1, TextureManager(),
  //                                                &texture_to_unbind));
  // EXPECT_NE(nullptr, texture_to_unbind);
  // EXPECT_TRUE(discardable_manager_.UnlockTexture(2, TextureManager(),
  //                                                &texture_to_unbind));
  // EXPECT_NE(nullptr, texture_to_unbind);
  // EXPECT_TRUE(discardable_manager_.UnlockTexture(4, TextureManager(),
  //                                                &texture_to_unbind));
  // EXPECT_NE(nullptr, texture_to_unbind);

  // // Allocate four more textures - the previous 4 should be evicted / deleted in
  // // LRU order.
  // {
  //   InSequence s;
  //   ExpectUnlockedTextureDeletion(3);
  //   ExpectUnlockedTextureDeletion(1);
  //   ExpectUnlockedTextureDeletion(2);
  //   ExpectUnlockedTextureDeletion(4);
  // }

  // for (int i = 5; i < 9; ++i) {
  //   TextureManager()->CreateTexture(i, i);
  //   auto handle = CreateLockedServiceHandleForTesting();
  //   discardable_manager_.InsertLockedTexture(i, texture_size, TextureManager(),
  //                                            handle);
  // }

  // // Ensure that the above expectations are handled by this point.
  // Mock::VerifyAndClearExpectations(gl_.get());
  // Mock::VerifyAndClearExpectations(&destruction_observer_);

  // // Unlock the next four textures:
  // EXPECT_TRUE(discardable_manager_.UnlockTexture(5, TextureManager(),
  //                                                &texture_to_unbind));
  // EXPECT_NE(nullptr, texture_to_unbind);
  // EXPECT_TRUE(discardable_manager_.UnlockTexture(6, TextureManager(),
  //                                                &texture_to_unbind));
  // EXPECT_NE(nullptr, texture_to_unbind);
  // EXPECT_TRUE(discardable_manager_.UnlockTexture(8, TextureManager(),
  //                                                &texture_to_unbind));
  // EXPECT_NE(nullptr, texture_to_unbind);
  // EXPECT_TRUE(discardable_manager_.UnlockTexture(7, TextureManager(),
  //                                                &texture_to_unbind));
  // EXPECT_NE(nullptr, texture_to_unbind);

  // // Allocate one more *large* texture, it should evict the LRU 3 textures.
  // {
  //   InSequence s;
  //   ExpectUnlockedTextureDeletion(5);
  //   ExpectUnlockedTextureDeletion(6);
  //   ExpectUnlockedTextureDeletion(8);
  // }

  // TextureManager()->CreateTexture(9, 9);
  // auto handle = CreateLockedServiceHandleForTesting();
  // discardable_manager_.InsertLockedTexture(9, large_texture_size,
  //                                          TextureManager(), handle);

  // // Expect the two remaining textures to clean up.
  // ExpectTextureDeletion(9);
  // ExpectUnlockedTextureDeletion(7);
}

TEST_F(ServiceDiscardableManagerTest, TextureSizeChanged) {
  const GLuint kClientId = 1;
  const GLuint kServiceId = 2;

  TextureManager()->CreateTexture(kClientId, kServiceId);
  TextureRef* texture_ref = TextureManager()->GetTexture(kClientId);
  auto handle = CreateLockedServiceHandleForTesting();
  discardable_manager_.InsertLockedTexture(kClientId, 0, TextureManager(),
                                           handle);
  EXPECT_EQ(0u, discardable_manager_.TotalSizeForTesting());
  TextureManager()->SetTarget(texture_ref, GL_TEXTURE_2D);
  TextureManager()->SetLevelInfo(texture_ref, GL_TEXTURE_2D, 0, GL_RGBA,
                                 kSmallTextureDim, kSmallTextureDim, 1, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE,
                                 gfx::Rect(kSmallTextureDim, kSmallTextureDim));
  EXPECT_EQ(kSmallTextureSize, discardable_manager_.TotalSizeForTesting());

  ExpectTextureDeletion(kClientId);
}

TEST_F(ServiceDiscardableManagerTest, OwnershipOnUnlock) {
  const GLuint kClientId = 1;
  const GLuint kServiceId = 2;

  std::unique_ptr<ServiceDiscardableHandle> service_handle;
  std::unique_ptr<ClientDiscardableHandle> client_handle;
  CreateLockedHandlesForTesting(&service_handle, &client_handle);
  TextureManager()->CreateTexture(kClientId, kServiceId);
  discardable_manager_.InsertLockedTexture(kClientId, kSmallTextureSize,
                                           TextureManager(), *service_handle);

  // Ensure that the service ref count is used to determine ownership changes.
  client_handle->Lock();
  TextureRef* texture_to_unbind;
  discardable_manager_.UnlockTexture(kClientId, TextureManager(),
                                     &texture_to_unbind);
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_TRUE(discardable_manager_.IsEntryLockedForTesting(kClientId,
                                                           TextureManager()));

  // Get the counts back in sync.
  discardable_manager_.LockTexture(kClientId, TextureManager());
  discardable_manager_.UnlockTexture(kClientId, TextureManager(),
                                     &texture_to_unbind);
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_FALSE(discardable_manager_.IsEntryLockedForTesting(kClientId,
                                                            TextureManager()));

  // Re-lock the texture twice.
  client_handle->Lock();
  discardable_manager_.LockTexture(kClientId, TextureManager());
  client_handle->Lock();
  discardable_manager_.LockTexture(kClientId, TextureManager());

  // Ensure that unlocking once doesn't cause us to unbind the texture.
  discardable_manager_.UnlockTexture(kClientId, TextureManager(),
                                     &texture_to_unbind);
  EXPECT_EQ(nullptr, texture_to_unbind);
  EXPECT_TRUE(discardable_manager_.IsEntryLockedForTesting(kClientId,
                                                           TextureManager()));

  // The second unlock should unbind/unlock the texture.
  discardable_manager_.UnlockTexture(kClientId, TextureManager(),
                                     &texture_to_unbind);
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_FALSE(discardable_manager_.IsEntryLockedForTesting(kClientId,
                                                            TextureManager()));

  ExpectUnlockedTextureDeletion(kClientId);
}

TEST_F(ServiceDiscardableManagerTest, BindGeneratedTextureLock) {
  const GLuint kClientId = 1;
  const GLuint kServiceId = 2;
  const GLuint kGeneratedServiceId = 3;

  // Create and insert a new texture.
  TextureManager()->CreateTexture(kClientId, kServiceId);
  auto handle = CreateLockedServiceHandleForTesting();
  discardable_manager_.InsertLockedTexture(kClientId, kSmallTextureSize,
                                           TextureManager(), handle);

  // Unlock the texture, ServiceDiscardableManager should take ownership of the
  // TextureRef.
  gles2::TextureRef* texture_to_unbind;
  EXPECT_TRUE(discardable_manager_.UnlockTexture(kClientId, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_EQ(nullptr, TextureManager()->GetTexture(kClientId));

  // Generate a new texture for the given client id, similar to "bind generates
  // resource" behavior.
  TextureManager()->CreateTexture(kClientId, kGeneratedServiceId);
  TextureRef* generated_texture_ref = TextureManager()->GetTexture(kClientId);

  // Re-lock the texture, the TextureManager should delete the returned
  // texture and keep the generated one.
  ExpectUnlockedTextureDeletion(kClientId);
  discardable_manager_.LockTexture(kClientId, TextureManager());
  EXPECT_EQ(generated_texture_ref, TextureManager()->GetTexture(kClientId));

  // Delete the texture from the TextureManager, it should also be removed from
  // the ServiceDiscardableManager.
  ExpectTextureDeletion(kClientId);
  TextureManager()->RemoveTexture(kClientId);
  EXPECT_EQ(0u, discardable_manager_.NumCacheEntriesForTesting());
}

TEST_F(ServiceDiscardableManagerTest, BindGeneratedTextureInitialization) {
  const GLuint kClientId = 1;
  const GLuint kServiceId = 2;
  const GLuint kGeneratedServiceId = 3;

  // Create and insert a new texture.
  TextureManager()->CreateTexture(kClientId, kServiceId);
  auto handle = CreateLockedServiceHandleForTesting();
  discardable_manager_.InsertLockedTexture(kClientId, kSmallTextureSize,
                                           TextureManager(), handle);

  // Unlock the texture, ServiceDiscardableManager should take ownership of the
  // TextureRef.
  gles2::TextureRef* texture_to_unbind;
  EXPECT_TRUE(discardable_manager_.UnlockTexture(kClientId, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_EQ(nullptr, TextureManager()->GetTexture(kClientId));

  // Generate a new texture for the given client id, similar to "bind generates
  // resource" behavior.
  TextureManager()->CreateTexture(kClientId, kGeneratedServiceId);
  TextureRef* generated_texture_ref = TextureManager()->GetTexture(kClientId);

  // Re-initialize the texture, the TextureManager should delete the old
  // texture and keep the generated one.
  ExpectUnlockedTextureDeletion(kClientId);
  discardable_manager_.InsertLockedTexture(kClientId, kSmallTextureSize,
                                           TextureManager(), handle);
  EXPECT_EQ(generated_texture_ref, TextureManager()->GetTexture(kClientId));

  ExpectTextureDeletion(kClientId);
}

TEST_F(ServiceDiscardableManagerTest, BindGeneratedTextureSizeChange) {
  const GLuint kClientId = 1;
  const GLuint kServiceId = 2;
  const GLuint kGeneratedServiceId = 3;

  // Create and insert a new texture.
  TextureManager()->CreateTexture(kClientId, kServiceId);
  auto handle = CreateLockedServiceHandleForTesting();
  discardable_manager_.InsertLockedTexture(kClientId, 0, TextureManager(),
                                           handle);

  // Unlock the texture, ServiceDiscardableManager should take ownership of the
  // TextureRef.
  gles2::TextureRef* texture_to_unbind;
  EXPECT_TRUE(discardable_manager_.UnlockTexture(kClientId, TextureManager(),
                                                 &texture_to_unbind));
  EXPECT_NE(nullptr, texture_to_unbind);
  EXPECT_EQ(nullptr, TextureManager()->GetTexture(kClientId));

  // Generate a new texture for the given client id, similar to "bind generates
  // resource" behavior.
  TextureManager()->CreateTexture(kClientId, kGeneratedServiceId);
  TextureRef* generated_texture_ref = TextureManager()->GetTexture(kClientId);

  // Re-size the generated texture. The tracked size should update.
  EXPECT_EQ(0u, discardable_manager_.TotalSizeForTesting());
  TextureManager()->SetTarget(generated_texture_ref, GL_TEXTURE_2D);
  TextureManager()->SetLevelInfo(generated_texture_ref, GL_TEXTURE_2D, 0,
                                 GL_RGBA, kSmallTextureDim, kSmallTextureDim, 1,
                                 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                 gfx::Rect(kSmallTextureDim, kSmallTextureDim));
  EXPECT_EQ(kSmallTextureSize, discardable_manager_.TotalSizeForTesting());

  ExpectUnlockedTextureDeletion(kClientId);
  ExpectTextureDeletion(kClientId);
}

}  // namespace gles2
}  // namespace gpu
