// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/media_session_controllers_manager.h"

#include "base/command_line.h"
#include "build/build_config.h"
#include "content/browser/media/session/media_session_controller.h"
#include "content/test/test_render_view_host.h"
#include "content/test/test_web_contents.h"
#include "media/base/media_content_type.h"
#include "media/base/media_switches.h"
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
    : public RenderViewHostImplTestHarness,
      public ::testing::WithParamInterface<std::tuple<bool, bool>> {
 public:
  // Indices of the tuple parameters.
  static const int kIsInternalMediaSessionEnabled = 0;
  static const int kIsAudioFocusEnabled = 1;

  void SetUp() override {
    RenderViewHostImplTestHarness::SetUp();

    std::string enabled_features;
    std::string disabled_features;

#if !defined(OS_ANDROID)
    if (IsInternalMediaSessionEnabled()) {
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          switches::kEnableInternalMediaSession);
    }
    if (IsAudioFocusEnabled()) {
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          switches::kEnableAudioFocus);
    }
#endif

    media_player_id_ = MediaSessionControllersManager::MediaPlayerId(
        contents()->GetMainFrame(), 1);
    mock_media_session_controller_ =
        std::make_unique<StrictMock<MockMediaSessionController>>(
            media_player_id_, contents()->media_web_contents_observer());
    mock_media_session_controller_ptr_ = mock_media_session_controller_.get();
    manager_ = std::make_unique<MediaSessionControllersManager>(
        contents()->media_web_contents_observer());
  }

  bool IsInternalMediaSessionEnabled() {
    return std::get<kIsInternalMediaSessionEnabled>(GetParam());
  }

  bool IsAudioFocusEnabled() {
    return std::get<kIsAudioFocusEnabled>(GetParam());
  }

  bool IsMediaSessionEnabled() {
#if defined(OS_ANDROID) || defined(OS_CHROMEOS)
    return true;
#else
    return IsInternalMediaSessionEnabled() || IsAudioFocusEnabled();
#endif
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
  std::unique_ptr<StrictMock<MockMediaSessionController>>
      mock_media_session_controller_;
  StrictMock<MockMediaSessionController>* mock_media_session_controller_ptr_ =
      nullptr;
  std::unique_ptr<MediaSessionControllersManager> manager_;
  MediaSessionControllersManager::ControllersMap* controllers_map_ = nullptr;
};

TEST_P(MediaSessionControllersManagerTest, RequestPlayAddsSessionsToMap) {
  EXPECT_TRUE(GetControllersMap()->empty());

  EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                    media::MediaContentType::Transient));
  if (!IsMediaSessionEnabled()) {
    EXPECT_TRUE(GetControllersMap()->empty());
  } else {
    EXPECT_EQ(1U, GetControllersMap()->size());
    EXPECT_TRUE(
        manager_->RequestPlay(MediaSessionControllersManager::MediaPlayerId(
                                  contents()->GetMainFrame(), 2),
                              true, false, media::MediaContentType::Transient));
    EXPECT_EQ(2U, GetControllersMap()->size());
  }
}

TEST_P(MediaSessionControllersManagerTest, RepeatAddsOfInitializablePlayer) {
  if (IsMediaSessionEnabled()) {
    EXPECT_TRUE(GetControllersMap()->empty());

    EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                      media::MediaContentType::Transient));
    EXPECT_EQ(1U, GetControllersMap()->size());

    EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                      media::MediaContentType::Transient));
    EXPECT_EQ(1U, GetControllersMap()->size());
  }
}

TEST_P(MediaSessionControllersManagerTest, RenderFrameDeletedRemovesHost) {
  EXPECT_TRUE(GetControllersMap()->empty());

  // Nothing should be removed if not enabled.
  if (!IsMediaSessionEnabled()) {
    // Artifically add controller to show early return.
    (*(GetControllersMap()))[media_player_id_] =
        std::move(mock_media_session_controller_);
    EXPECT_EQ(1U, GetControllersMap()->size());

    manager_->RenderFrameDeleted(contents()->GetMainFrame());
    EXPECT_EQ(1U, GetControllersMap()->size());
  } else {
    EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                      media::MediaContentType::Transient));
    EXPECT_EQ(1U, controllers_map_->size());

    manager_->RenderFrameDeleted(contents()->GetMainFrame());
    EXPECT_TRUE(GetControllersMap()->empty());
  }
}

TEST_P(MediaSessionControllersManagerTest, OnPauseCallsPlaybackPaused) {
  // Artifically add controller to show early return.
  (*(GetControllersMap()))[media_player_id_] =
      std::move(mock_media_session_controller_);
  if (IsMediaSessionEnabled())
    EXPECT_CALL(*mock_media_session_controller_ptr_, OnPlaybackPaused());

  manager_->OnPause(media_player_id_);
}

TEST_P(MediaSessionControllersManagerTest, OnPauseIdNotFound) {
  if (IsMediaSessionEnabled()) {
    // Artifically add controller to show early return.
    (*(GetControllersMap()))[media_player_id_] =
        std::move(mock_media_session_controller_);

    MediaSessionControllersManager::MediaPlayerId id2 =
        MediaSessionControllersManager::MediaPlayerId(
            contents()->GetMainFrame(), 2);
    manager_->OnPause(id2);
  }
}

TEST_P(MediaSessionControllersManagerTest, OnEndRemovesMediaPlayerId) {
  EXPECT_TRUE(GetControllersMap()->empty());

  // No op if not enabled.
  if (!IsMediaSessionEnabled()) {
    // Artifically add controller to show early return.
    (*(GetControllersMap()))[media_player_id_] =
        std::move(mock_media_session_controller_);
    EXPECT_EQ(1U, GetControllersMap()->size());

    manager_->OnEnd(media_player_id_);
    EXPECT_EQ(1U, GetControllersMap()->size());
  } else {
    EXPECT_TRUE(manager_->RequestPlay(media_player_id_, true, false,
                                      media::MediaContentType::Transient));
    EXPECT_EQ(1U, GetControllersMap()->size());

    manager_->OnEnd(media_player_id_);
    EXPECT_TRUE(GetControllersMap()->empty());
  }
}

INSTANTIATE_TEST_CASE_P(MediaSessionEnabledTestInstances,
                        MediaSessionControllersManagerTest,
                        ::testing::Combine(::testing::Bool(),
                                           ::testing::Bool()));
}  // namespace content
