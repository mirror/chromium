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
#include "build/build_config.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ScopedTaskEnvironment;

namespace content {

namespace {
struct PeerConnectionKey {
  int render_process_id;
  int lid;  // Renderer-local PeerConnection ID.
};
}  // namespace

class WebRtcEventLogManagerTest : public ::testing::Test {
 protected:
  WebRtcEventLogManagerTest()
      : run_loop_(new base::RunLoop),
        manager_(base::MakeUnique<WebRtcEventLogManager>()) {
    EXPECT_TRUE(
        base::CreateNewTempDirectory(FILE_PATH_LITERAL(""), &base_dir_));
    if (base_dir_.empty()) {
      return;
    }
    base_path_ = base_dir_.Append(FILE_PATH_LITERAL("webrtc_event_log"));
    test_clock_.SetNow(base::Time::Now());
    manager_->InjectClockForTesting(&test_clock_);
  }

  ~WebRtcEventLogManagerTest() override {
    if (!base_dir_.empty()) {
      EXPECT_TRUE(base::DeleteFile(base_dir_, true));
    }
  }

  base::FilePath ExpectedLocalWebRtcEventLogFilePath(int render_process_id,
                                                     int local_peer_id) {
    return manager_->GetLocalFilePath(base_path_, render_process_id,
                                      local_peer_id);
  }

  void BlockTest() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop);  // Allow re-blocking.
  }

  base::Closure UnblockTest() { return run_loop_->QuitWhenIdleClosure(); }

  static constexpr int kRenderProcessId = 23;
  static constexpr int kLocalPeerConnectionId = 478;
  static constexpr size_t kMaxFileSizeBytes = 50000;

  // Testing utilities.
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  base::SimpleTestClock test_clock_;
  std::unique_ptr<base::RunLoop> run_loop_;  // New instance allows reblocking.

  std::unique_ptr<WebRtcEventLogManager> manager_;  // Object under test.

  base::FilePath base_dir_;   // The directory where we'll save log files.
  base::FilePath base_path_;  // base_dir_ +  log files' name prefix.
};

TEST_F(WebRtcEventLogManagerTest, LocalLogCreateEmptyFile) {
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, kMaxFileSizeBytes);
  manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                                    UnblockTest());

  BlockTest();

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
                                    UnblockTest());

  BlockTest();

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
                                    UnblockTest());

  BlockTest();

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
                                    UnblockTest());

  BlockTest();

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
                                    UnblockTest());

  BlockTest();

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
                                  log_after, UnblockTest());

  // Note that it doesn't matter if Stop(), which is async, has executed or
  // not by the time that the second Write() executes. We keep the code written
  // as it is so that it would cover both cases (though only one each time).
  BlockTest();

  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents, log_before);
}

// This is a bit of a subset of LocalLogOnlyWritesTheLogsAfterStart, but is
// kept as a separate test for clarity.
TEST_F(WebRtcEventLogManagerTest, LocalLogDoesNotWriteToNeverStartedLog) {
  const std::string log = "The lights begin to twinkle from the rocks:";
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log,
                                  UnblockTest());
  BlockTest();
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogOnlyWritesTheLogsAfterStart) {
  // Calls to Write() before Start() are ignored.
  const std::string log1 = "The lights begin to twinkle from the rocks:";
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                  log1, UnblockTest());
  BlockTest();
  ASSERT_TRUE(base::IsDirectoryEmpty(base_dir_));

  // Calls after Start() have an effect. The calls to Write() from before
  // Start() are not remembered.
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, kMaxFileSizeBytes);
  const std::string log2 = "The long day wanes: the slow moon climbs: the deep";
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                  log2, UnblockTest());
  BlockTest();
  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents, log2);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogStopBeforeStartHasNoEffect) {
  // Calls to Stop() before Start() are ignored.
  manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                                    UnblockTest());
  BlockTest();
  ASSERT_TRUE(base::IsDirectoryEmpty(base_dir_));

  // The Stop() before does not leave any bad state behind. We can still
  // Start() the log, Write() to it and Close() it.
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, kMaxFileSizeBytes);
  const std::string log = "To err is canine; to forgive, feline.";
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log,
                                  UnblockTest());
  BlockTest();
  const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
      kRenderProcessId, kLocalPeerConnectionId);
  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
  EXPECT_EQ(file_contents, log);
}

// Note: This test also covers the scenario LocalLogExistingFilesNotOverwritten,
// which is therefore not explicitly tested.
TEST_F(WebRtcEventLogManagerTest, LocalLogRestartedLogCreatesNewFile) {
  const std::string logs[] = {"<joke setup>", "<punchline>", "encore"};
  for (size_t i = 0; i < arraysize(logs); i++) {
    manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                       base_path_, kMaxFileSizeBytes);
    manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                    logs[i]);
    auto reply =
        (i < arraysize(logs) - 1) ? base::OnceClosure() : UnblockTest();
    manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                                      std::move(reply));
  }

  BlockTest();

  for (size_t i = 0; i < arraysize(logs); i++) {
    base::FilePath expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
        kRenderProcessId, kLocalPeerConnectionId);
    if (i > 0) {
      expected_file_path = expected_file_path.InsertBeforeExtension(
          " (" + std::to_string(i) + ")");
    }
    std::string file_contents;
    EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
    EXPECT_EQ(file_contents, logs[i]);
  }
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleActiveFiles) {
  const PeerConnectionKey keys[] = {{1, 2}, {2, 1}, {3, 4},
                                    {4, 3}, {5, 5}, {6, 7}};
  std::vector<std::string> logs;

  for (const auto& key : keys) {
    manager_->LocalWebRtcEventLogStart(key.render_process_id, key.lid,
                                       base_path_, kMaxFileSizeBytes);
  }

  for (size_t i = 0; i < arraysize(keys); i++) {
    logs.emplace_back(std::to_string(kRenderProcessId) +
                      std::to_string(kLocalPeerConnectionId));
    manager_->OnWebRtcEventLogWrite(keys[i].render_process_id, keys[i].lid,
                                    logs[i]);
  }

  for (size_t i = 0; i < arraysize(keys); i++) {
    auto reply =
        (i < arraysize(keys) - 1) ? base::OnceClosure() : UnblockTest();
    manager_->LocalWebRtcEventLogStop(keys[i].render_process_id, keys[i].lid,
                                      std::move(reply));
  }

  BlockTest();

  for (size_t i = 0; i < arraysize(keys); i++) {
    const auto expected_file_path = ExpectedLocalWebRtcEventLogFilePath(
        keys[i].render_process_id, keys[i].lid);
    std::string file_contents;
    EXPECT_TRUE(base::ReadFileToString(expected_file_path, &file_contents));
    EXPECT_EQ(file_contents, logs[i]);
  }
}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityDestructorWithActiveFile) {
  // We don't actually test that the file was closed, because this behavior
  // is not supported - WebRtcEventLogManager's destructor may never be called,
  // except for in unit-tests.
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, kMaxFileSizeBytes,
                                     UnblockTest());
  BlockTest();
  manager_.reset();  // Explicitly show destruction does not crash.
}

TEST_F(WebRtcEventLogManagerTest, LocalLogIllegalPath) {
  const base::FilePath illegal_path(FILE_PATH_LITERAL("!@#$%|`^&*\\/"));
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     illegal_path, kMaxFileSizeBytes,
                                     UnblockTest());
  BlockTest();
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));
}

// TODO(eladalon): It would be nice to also have a version of this test for
// non-POSIX platforms.
#if defined(OS_POSIX)
TEST_F(WebRtcEventLogManagerTest, LocalLogLegalPathWithoutPermissionsSanity) {
  int permissions;
  ASSERT_TRUE(base::GetPosixFilePermissions(base_dir_, &permissions));

  constexpr int write_permissions = base::FILE_PERMISSION_WRITE_BY_USER |
                                    base::FILE_PERMISSION_WRITE_BY_GROUP |
                                    base::FILE_PERMISSION_WRITE_BY_OTHERS;
  permissions &= ~write_permissions;

  ASSERT_TRUE(base::SetPosixFilePermissions(base_dir_, permissions));

  // Start() has no effect (but is handled gracefully).
  manager_->LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                     base_path_, kMaxFileSizeBytes,
                                     UnblockTest());
  BlockTest();
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));

  // Write() has no effect (but is handled gracefully).
  manager_->OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                  "Why did the chicken cross the road?",
                                  UnblockTest());
  BlockTest();
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));

  // Stop() has no effect (but is handled gracefully).
  manager_->LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                                    UnblockTest());
  BlockTest();
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));
}
#endif

}  // namespace content
