// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

// TODO(eladalon): !!! Finish the unit-tests (before landing).

using base::test::ScopedTaskEnvironment;

namespace content {

class WebRtcEventLogManagerTest
    : public ::testing::Test,
      public WebRtcEventLogManager::FileClosureObserverForTesting {
 public:
  WebRtcEventLogManagerTest()
      : manager_(base::MakeUnique<WebRtcEventLogManager>()),
        file_closed_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                           base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  void SetUp() override {
    EXPECT_TRUE(base::CreateNewTempDirectory("", &base_dir_));
    test_clock_.SetNow(base::Time::Now());
    manager_->InjectClockForTesting(&test_clock_);
    manager_->SetObserverForTesting(this);
  }

  ~WebRtcEventLogManagerTest() override {
    if (!base_dir_.empty()) {
      EXPECT_TRUE(base::DeleteFile(base_dir_, true));
    }
  }

  void OnFileClosed(int render_process_id, int local_peer_id) override {
    auto key = PeerConnectionKey{render_process_id, local_peer_id};
    auto it = std::find(files_waiting_.begin(), files_waiting_.end(), key);
    if (it != files_waiting_.end()) {
      files_waiting_.erase(it);
      if (files_waiting_.empty()) {
        file_closed_event_.Signal();
      }
    }
  }

 protected:
  base::FilePath ExpectedLocalWebRtcEventLogFilePath(int render_process_id,
                                                     int local_peer_id) {
    return manager_->GetLocalFilePath(base_dir_, render_process_id,
                                      local_peer_id);
  }

  void LocalWebRtcEventLogStart(int render_process_id,
                                int local_peer_id,
                                size_t max_log_file_size_bytes) {
    files_waiting_.push_back({render_process_id, local_peer_id});
    manager_->LocalWebRtcEventLogStart(render_process_id, local_peer_id,
                                       base_dir_, max_log_file_size_bytes);
  }

  void LocalWebRtcEventLogStop(int render_process_id, int local_peer_id) {
    manager_->LocalWebRtcEventLogStop(render_process_id, local_peer_id);
  }

  static constexpr int kArbitraryRenderProcessId = 23;
  static constexpr int kArbitraryLocalPeerConnectionId = 478;
  static constexpr size_t kArbitraryMaxFileSizeBytes = 50000;

  const base::TimeDelta kTimeout = base::TimeDelta::FromSeconds(10);

  content::TestBrowserThreadBundle test_browser_thread_bundle_;

  base::SimpleTestClock test_clock_;
  std::unique_ptr<WebRtcEventLogManager> manager_;

  base::FilePath base_dir_;

  struct PeerConnectionKey {
    bool operator==(const PeerConnectionKey& other) const {
      return std::tie(render_process_id, local_peer_id) ==
             std::tie(other.render_process_id, other.local_peer_id);
    }
    int render_process_id;
    int local_peer_id;
  };
  std::vector<PeerConnectionKey> files_waiting_;
  base::WaitableEvent file_closed_event_;
};

TEST_F(WebRtcEventLogManagerTest, LocalLogCreateEmptyFile) {
  LocalWebRtcEventLogStart(kArbitraryRenderProcessId,
                           kArbitraryLocalPeerConnectionId,
                           kArbitraryMaxFileSizeBytes);
  LocalWebRtcEventLogStop(kArbitraryRenderProcessId,
                          kArbitraryLocalPeerConnectionId);

  ASSERT_TRUE(file_closed_event_.TimedWait(kTimeout));

  auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kArbitraryRenderProcessId, kArbitraryLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents.length(), 0u);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogCreateAndWriteToFile) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleWritesToSameFile) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogFileSizeLimitNotExceeded) {}

// It would be infeasible to write "without limit", but one can show that sane
// behavior is encountered for the first 1MB (as opposed to no information being
// written to the file, for instance).
TEST_F(WebRtcEventLogManagerTest, LocalLogSanityOverUnlimitedFileSizes) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogNoWriteAfterLogStop) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityOverNeverStartedLog) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogRestartedLogEndsInNewFile) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleActiveFiles) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityDestructorWithActiveFile) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogIllegalPath) {}
