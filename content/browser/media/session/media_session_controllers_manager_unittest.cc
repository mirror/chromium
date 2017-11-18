// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/media_session_controllers_manager.h"
#include "content/browser/media/session/media_session_controller.h"
#include "content/test/test_render_view_host.h"
#include "content/test/test_web_contents.h"
#include "media/base/media_content_type.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::StrictMock;

namespace content {

namespace {

class MockMediaSessionController : public MediaSessionController {
 public:
  MockMediaSessionController(
      const WebContentsObserver::MediaPlayerId& id,
      MediaWebContentsObserver* media_web_contents_observer)
      : MediaSessionController(id, media_web_contents_observer) {}

  MOCK_METHOD0(OnPlaybackPaused, void());
};

}  // namespace

class MediaSessionControllersManagerTest
    : public RenderViewHostImplTestHarness {
 public:
  void SetUp() override {
    RenderViewHostImplTestHarness::SetUp();
    media_player_id_ = MediaSessionControllersManager::MediaPlayerId(
        contents()->GetMainFrame(), 1);
    mock_media_session_controller_ = new StrictMock<MockMediaSessionController>(
        media_player_id_, contents()->media_web_contents_observer());
  }

  MediaSessionControllersManager::ControllersMap* GetControllersMap() {
    if (!controllers_map_)
      controllers_map_ = &(manager_->controllers_map_);
    return controllers_map_;
  }

  void TearDown() override {
    manager_.reset();
    RenderViewHostImplTestHarness::TearDown();
  }

 protected:
  MediaSessionControllersManager::MediaPlayerId media_player_id_;
  StrictMock<MockMediaSessionController>* mock_media_session_controller_ =
      nullptr;
  std::unique_ptr<MediaSessionControllersManager> manager_;
  MediaSessionControllersManager::ControllersMap* controllers_map_ = nullptr;
};

TEST_F(MediaSessionControllersManagerTest, RequestPlayNotEnabledReturnsTrue) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), false);
  EXPECT_TRUE(GetControllersMap()->empty());

  EXPECT_TRUE(manager_->RequestPlay(media_player_id_, false, false,
                                    media::MediaContentType::Persistent));
  EXPECT_TRUE(GetControllersMap()->empty());
}

TEST_F(MediaSessionControllersManagerTest, NewMediaPlayersAddSessionsToMap) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), true);
  EXPECT_TRUE(GetControllersMap()->empty());

  EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                    media::MediaContentType::Transient));
  EXPECT_EQ(1U, GetControllersMap()->size());

  EXPECT_TRUE(
      manager_->RequestPlay(MediaSessionControllersManager::MediaPlayerId(
                                contents()->GetMainFrame(), 2),
                            true, false, media::MediaContentType::Transient));
  EXPECT_EQ(2U, GetControllersMap()->size());
}

TEST_F(MediaSessionControllersManagerTest, RepeatAddsOfInitializablePlayer) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), true);
  EXPECT_TRUE(GetControllersMap()->empty());

  EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                    media::MediaContentType::Transient));
  EXPECT_EQ(1U, GetControllersMap()->size());

  EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                    media::MediaContentType::Transient));
  EXPECT_EQ(1U, GetControllersMap()->size());
}

TEST_F(MediaSessionControllersManagerTest, RenderFrameDeletedNotEnabledNoOps) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), false);
  EXPECT_TRUE(GetControllersMap()->empty());

  // Artifically add controller to show early return.
  (*(GetControllersMap()))[media_player_id_] =
      base::WrapUnique<MockMediaSessionController>(
          mock_media_session_controller_);
  EXPECT_EQ(1U, GetControllersMap()->size());

  manager_->RenderFrameDeleted(contents()->GetMainFrame());
  EXPECT_EQ(1U, GetControllersMap()->size());
}

TEST_F(MediaSessionControllersManagerTest, RenderFrameDeletedRemovesHost) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), true);
  EXPECT_TRUE(GetControllersMap()->empty());

  EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                    media::MediaContentType::Transient));
  EXPECT_EQ(1U, controllers_map_->size());

  manager_->RenderFrameDeleted(contents()->GetMainFrame());
  EXPECT_TRUE(GetControllersMap()->empty());
}

TEST_F(MediaSessionControllersManagerTest, OnPauseNotEnabledNoOps) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), false);

  // Artifically add controller to show early return.
  (*(GetControllersMap()))[media_player_id_] =
      base::WrapUnique<MockMediaSessionController>(
          mock_media_session_controller_);
  manager_->OnPause(media_player_id_);
}

TEST_F(MediaSessionControllersManagerTest, OnPauseIdNotFound) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), true);

  // Artifically add controller to show early return.
  (*(GetControllersMap()))[media_player_id_] =
      base::WrapUnique<MockMediaSessionController>(
          mock_media_session_controller_);

  MediaSessionControllersManager::MediaPlayerId id2 =
      MediaSessionControllersManager::MediaPlayerId(contents()->GetMainFrame(),
                                                    2);
  manager_->OnPause(id2);
}

TEST_F(MediaSessionControllersManagerTest, OnPauseCallsOnPlaybackPaused) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), true);

  // Artifically inject mock controller.
  (*(GetControllersMap()))[media_player_id_] =
      base::WrapUnique<MockMediaSessionController>(
          mock_media_session_controller_);

  EXPECT_CALL(*mock_media_session_controller_, OnPlaybackPaused());
  manager_->OnPause(media_player_id_);
}

TEST_F(MediaSessionControllersManagerTest, OnEndNotEnabledNoOps) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), false);
  EXPECT_TRUE(GetControllersMap()->empty());

  // Artifically add controller to show early return.
  (*(GetControllersMap()))[media_player_id_] =
      base::WrapUnique<MockMediaSessionController>(
          mock_media_session_controller_);
  EXPECT_EQ(1U, GetControllersMap()->size());

  manager_->OnEnd(media_player_id_);
  EXPECT_EQ(1U, GetControllersMap()->size());
}

TEST_F(MediaSessionControllersManagerTest, OnEndRemovesMediaPlayerId) {
  manager_ = std::make_unique<MediaSessionControllersManager>(
      contents()->media_web_contents_observer(), true);
  EXPECT_TRUE(GetControllersMap()->empty());

  EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                    media::MediaContentType::Transient));
  EXPECT_EQ(1U, GetControllersMap()->size());

  manager_->OnEnd(media_player_id_);
  EXPECT_TRUE(GetControllersMap()->empty());
}

}  // namespace content
