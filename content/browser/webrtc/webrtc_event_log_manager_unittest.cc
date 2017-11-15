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

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

// TODO(eladalon): !!! Finish the unit-tests (before landing).

// TODO(eladalon): !!! run_loop_.Run(); - can we bound this?

using base::test::ScopedTaskEnvironment;

namespace content {

class WebRtcEventLogManagerTest : public ::testing::Test {
 public:
  WebRtcEventLogManagerTest()
      : manager_(base::MakeUnique<WebRtcEventLogManager>()) {
    EXPECT_TRUE(
        base::CreateNewTempDirectory(FILE_PATH_LITERAL(""), &base_dir_));
    base_path_ = base_dir_.Append(FILE_PATH_LITERAL("webrtc_event_log"));
    test_clock_.SetNow(base::Time::Now());
    manager_->InjectClockForTesting(&test_clock_);
  }

  ~WebRtcEventLogManagerTest() override {
    if (!base_dir_.empty()) {  // TODO(eladalon): !!!
      EXPECT_TRUE(base::DeleteFile(base_dir_, true));
    }
  }

 protected:
  base::FilePath ExpectedLocalWebRtcEventLogFilePath(int render_process_id,
                                                     int local_peer_id) {
    return manager_->GetLocalFilePath(base_path_, render_process_id,
                                      local_peer_id);
  }

  static constexpr int kRenderProcessId = 23;
  static constexpr int kLocalPeerConnectionId = 478;
  static constexpr size_t kMaxFileSizeBytes = 50000;

  const base::TimeDelta kTimeout = base::TimeDelta::FromSeconds(1);

  content::TestBrowserThreadBundle test_browser_thread_bundle_;

  base::SimpleTestClock test_clock_;
  std::unique_ptr<WebRtcEventLogManager> manager_;

  base::FilePath base_dir_;
  base::FilePath base_path_;

  base::RunLoop run_loop_;
};

TEST_F(WebRtcEventLogManagerTest, LocalLogCreateEmptyFile) {
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, kMaxFileSizeBytes);
  manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                                    run_loop_.QuitWhenIdleClosure());

  run_loop_.Run();  // Wait for the last async call to |manager_| to finish.

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents, "");
}

TEST_F(WebRtcEventLogManagerTest, LocalLogCreateAndWriteToFile) {
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, kMaxFileSizeBytes);

  const std::string log = "To strive, to seek, to find, and not to yield.";
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                  log);

  manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                                    run_loop_.QuitWhenIdleClosure());

  run_loop_.Run();  // Wait for the last async call to |manager_| to finish.

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents, log);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleWritesToSameFile) {
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, kMaxFileSizeBytes);

  const std::string logs[] = {"Old age hath yet his honour and his toil;",
                              "Death closes all: but something ere the end,",
                              "Some work of noble note, may yet be done,",
                              "Not unbecoming men that strove with Gods."};

  for (const std::string& log : logs) {
    manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                    log);
  }

  manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                                    run_loop_.QuitWhenIdleClosure());

  run_loop_.Run();  // Wait for the last async call to |manager_| to finish.

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents,
            std::accumulate(std::begin(logs), std::end(logs), std::string()));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogFileSizeLimitNotExceeded) {
  const std::string log = "There lies the port; the vessel puffs her sail:";
  const size_t file_size_limit = log.length() / 2;

  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, file_size_limit);

  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                  log);

  manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                                    run_loop_.QuitWhenIdleClosure());

  run_loop_.Run();  // Wait for the last async call to |manager_| to finish.

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents, log.substr(0, file_size_limit));
}

// It would be infeasible to write "without limit", but one can show that the
// expected behavior is encountered for relatively big outputs. Since this
// will run many times, along with many other tests, we'll not even try
// relatively big ones, though.
TEST_F(WebRtcEventLogManagerTest, LocalLogSanityOverUnlimitedFileSizes) {
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_,
                                     WebRtcEventLogManager::kUnlimitedFileSize);

  std::stringstream ss;
  for (size_t i = 0; i < 1000000u; i++) {
    ss << i;
  }
  const std::string log = ss.str();
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                  log);

  manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                                    run_loop_.QuitWhenIdleClosure());

  run_loop_.Run();  // Wait for the last async call to |manager_| to finish.

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));

  bool input_equals_output = (file_contents == log);  // Avoid long error msg.
  EXPECT_TRUE(input_equals_output);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogNoWriteAfterLogStop) {
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, kMaxFileSizeBytes);

  const std::string log_before = "log_before_stop";
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                  log_before);

  manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId);

  const std::string log_after = "log_after_stop";
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                  log_after, run_loop_.QuitWhenIdleClosure());

  // Wait for the last async call to |manager_| to finish.
  // Note that it doesn't matter if Stop(), which is async, has executed or
  // not by the time that the second Write() executes. We keep the code written
  // as it is so that it would cover both cases (though only one each time).
  run_loop_.Run();

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents, log_before);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityOverNeverStartedLog) {
  const std::string log = "The lights begin to twinkle from the rocks:";
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log,
                                  run_loop_.QuitWhenIdleClosure());

  run_loop_.Run();  // Wait for the last async call to |manager_| to finish.

  // Impossible to wait for the closing of a file that should never be
  // created nor closed. If this were to ever fail, it would do so in a
  // flaky manner.
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogRestartedLogCreatesNewFile) {
//  const std::string logs[] = {"<joke setup>", "<punchline>"};
//  for (size_t i = 0; i < arraysize(logs); i++) {
//    manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
//                                       base_path_, kMaxFileSizeBytes);
//
//    manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
//                                    logs[i]);
//
//    manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
//                                      run_loop_.QuitWhenIdleClosure());
//  }
//
//  run_loop_.Run();  // Wait for the last async call to |manager_| to finish.
//
//  for (size_t i = 0; i < arraysize(logs); i++) {
//    base::FilePath expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
//        kRenderProcessId, kLocalPeerConnectionId);
//    if (i > 0) {
//      expected_file_path =
//          expected_file_path.InsertBeforeExtension("_" + std::to_string(i));
//    }
//    std::string file_contents;
//    EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
//    EXPECT_EQ(file_contents, logs[i]);
//  }
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleActiveFiles) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityDestructorWithActiveFile) {}

TEST_F(WebRtcEventLogManagerTest, LocalLogIllegalPath) {}

}  // namespace content
