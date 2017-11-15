// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
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
      public WebRtcEventLogManager::FileObserverForTesting {
 public:
  WebRtcEventLogManagerTest()
      : manager_(base::MakeUnique<WebRtcEventLogManager>()),
        all_files_closed_(base::WaitableEvent::ResetPolicy::MANUAL,
                          base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  void SetUp() override {
    EXPECT_TRUE(base::CreateNewTempDirectory("", &base_dir_));
    base_dir_ = base_dir_.Append("webrtc_event_log");
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
    auto it = std::find(pending_files_.begin(), pending_files_.end(), key);
    if (it != pending_files_.end()) {
      pending_files_.erase(it);
      if (pending_files_.empty()) {
        all_files_closed_.Signal();
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
    pending_files_.push_back({render_process_id, local_peer_id});
    manager_->LocalWebRtcEventLogStart(render_process_id, local_peer_id,
                                       base_dir_, max_log_file_size_bytes);
  }

  void LocalWebRtcEventLogStop(int render_process_id, int local_peer_id) {
    manager_->LocalWebRtcEventLogStop(render_process_id, local_peer_id);
  }

  void OnWebRtcEventLogWrite(int render_process_id,
                             int local_peer_id,
                             const std::string& output) {
    manager_->OnWebRtcEventLogWrite(render_process_id, local_peer_id, output);
  }

  static constexpr int kRenderProcessId = 23;
  static constexpr int kLocalPeerConnectionId = 478;
  static constexpr size_t kMaxFileSizeBytes = 50000;

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
  std::vector<PeerConnectionKey> pending_files_;

  base::WaitableEvent all_files_closed_;
};

TEST_F(WebRtcEventLogManagerTest, LocalLogCreateEmptyFile) {
  LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                           kMaxFileSizeBytes);
  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId);

  ASSERT_TRUE(all_files_closed_.TimedWait(kTimeout));

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents, "");
}

TEST_F(WebRtcEventLogManagerTest, LocalLogCreateAndWriteToFile) {
//  LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
//                           kMaxFileSizeBytes);
//
//  const std::string log = "To strive, to seek, to find, and not to yield.";
//  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log);
//
//  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId);
//
//  ASSERT_TRUE(all_files_closed_.TimedWait(kTimeout));
//
//  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
//      kRenderProcessId, kLocalPeerConnectionId);
//  std::string file_contents;
//  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
//  EXPECT_EQ(file_contents, log);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleWritesToSameFile) {
  LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                           kMaxFileSizeBytes);

  const std::string logs[] = {"Old age hath yet his honour and his toil;",
                              "Death closes all: but something ere the end,",
                              "Some work of noble note, may yet be done,",
                              "Not unbecoming men that strove with Gods."};

  for (const std::string& log : logs) {
    OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log);
  }

  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId);

  ASSERT_TRUE(all_files_closed_.TimedWait(kTimeout));

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents,
            std::accumulate(std::begin(logs), std::end(logs), std::string()));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogFileSizeLimitNotExceeded) {
//  const std::string log = "There lies the port; the vessel puffs her sail:";
//  const size_t file_size_limit = log.length() / 2;
//
//  LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
//                           file_size_limit);
//
//  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log);
//
//  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId);
//
//  ASSERT_TRUE(all_files_closed_.TimedWait(kTimeout));
//
//  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
//      kRenderProcessId, kLocalPeerConnectionId);
//  std::string file_contents;
//  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
//  EXPECT_EQ(file_contents, log.substr(0, file_size_limit));
}

// It would be infeasible to write "without limit", but one can show that the
// expected behavior is encountered for relatively big outputs. Since this
// will run many times, along with many other tests, we'll not even try
// relatively big ones, though.
TEST_F(WebRtcEventLogManagerTest, LocalLogSanityOverUnlimitedFileSizes) {
  LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                           WebRtcEventLogManager::kUnlimitedFileSize);

  std::stringstream ss;
  for (size_t i = 0; i < 1000000u; i++) {
    ss << i;
  }
  const std::string log = ss.str();
  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log);

  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId);

  ASSERT_TRUE(all_files_closed_.TimedWait(kTimeout));

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));

  bool input_equals_output = (file_contents == log);  // Avoid long error msg.
  EXPECT_TRUE(input_equals_output);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogNoWriteAfterLogStop) {
//  LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
//                           kMaxFileSizeBytes);
//
//  const std::string log_before = "log_before_stop";
//  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log_before);
//
//  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId);
//
//  const std::string log_after = "log_after_stop";
//  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log_after);
//
//  ASSERT_TRUE(all_files_closed_.TimedWait(kTimeout));
//
//  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
//      kRenderProcessId, kLocalPeerConnectionId);
//  std::string file_contents;
//  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
//  EXPECT_EQ(file_contents, log_before);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityOverNeverStartedLog) {
//  const std::string log = "The lights begin to twinkle from the rocks:";
//  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log);
//
//  // Impossible to wait for the closing of a file that should never be
//  // created nor closed. If this were to ever fail, it would do so in a
//  // flaky manner.
//  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogRestartedLogCreatesNewFile) {
  const std::string logs[] = {"<joke setup>", "<punchline>"};
  for (size_t i = 0; i < arraysize(logs); i++) {
    LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                             kMaxFileSizeBytes);

    OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, logs[i]);

    LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId);
  }

  ASSERT_TRUE(all_files_closed_.TimedWait(kTimeout));

  for (size_t i = 0; i < arraysize(logs); i++) {
    base::FilePath expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
        kRenderProcessId, kLocalPeerConnectionId);
    if (i > 0) {
      expected_file_path =
          expected_file_path.InsertBeforeExtension("_" + std::to_string(i));
    }
    std::string file_contents;
    EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
    EXPECT_EQ(file_contents, logs[i]);
  }
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleActiveFiles) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityDestructorWithActiveFile) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogIllegalPath) {}

}  // namespace content
