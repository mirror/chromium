// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/gtest_util.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
#define IntToStringType base::IntToString16
#else
#define IntToStringType base::IntToString
#endif

namespace content {

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using PeerConnectionKey = WebRtcEventLogManager::PeerConnectionKey;

namespace {

class MockLocalLogsObserver : public WebRtcEventLogManager::LocalLogsObserver {
 public:
  ~MockLocalLogsObserver() override = default;
  MOCK_METHOD2(OnLocalLogsStarted, void(PeerConnectionKey, base::FilePath));
  MOCK_METHOD1(OnLocalLogsStopped, void(PeerConnectionKey));
};

class MemorizingLocalLogsObserver
    : public WebRtcEventLogManager::LocalLogsObserver {
 public:
  ~MemorizingLocalLogsObserver() override = default;

  void OnLocalLogsStarted(PeerConnectionKey peer_connection,
                          base::FilePath file_path) override {
    EXPECT_EQ(started_local_logs_.find(peer_connection),
              started_local_logs_.end());
    started_local_logs_.emplace(peer_connection, file_path);
  }

  void OnLocalLogsStopped(PeerConnectionKey peer_connection) override {
    EXPECT_EQ(stopped_local_logs_.find(peer_connection),
              stopped_local_logs_.end());
    stopped_local_logs_.insert(peer_connection);
  }

  std::map<PeerConnectionKey, base::FilePath> GetStartedLogs() {
    std::map<PeerConnectionKey, base::FilePath> result;
    std::swap(started_local_logs_, result);
    return result;
  }

  std::set<PeerConnectionKey> GetStoppedLogs() {
    std::set<PeerConnectionKey> result;
    std::swap(stopped_local_logs_, result);
    return result;
  }

 protected:
  std::map<PeerConnectionKey, base::FilePath> started_local_logs_;
  std::set<PeerConnectionKey> stopped_local_logs_;
};

auto SaveKeyAndFilePathTo(base::Optional<PeerConnectionKey>* key_output,
                          base::Optional<base::FilePath>* file_path_output) {
  return [key_output, file_path_output](PeerConnectionKey key,
                                        base::FilePath file_path) {
    *key_output = key;
    *file_path_output = file_path;
  };
}

}  // namespace

class WebRtcEventLogManagerTest : public ::testing::Test {
 protected:
  WebRtcEventLogManagerTest()
      : run_loop_(base::MakeUnique<base::RunLoop>()),
        manager_(new WebRtcEventLogManager) {
    EXPECT_TRUE(
        base::CreateNewTempDirectory(FILE_PATH_LITERAL(""), &base_dir_));
    if (base_dir_.empty()) {
      EXPECT_TRUE(false);
      return;
    }
    base_path_ = base_dir_.Append(FILE_PATH_LITERAL("webrtc_event_log"));
  }

  ~WebRtcEventLogManagerTest() override {
    SetLocalLogsObserver(nullptr);
    DestroyUnitUnderTest();
    if (!base_dir_.empty()) {
      EXPECT_TRUE(base::DeleteFile(base_dir_, true));
    }
  }

  void DestroyUnitUnderTest() {
    if (manager_ != nullptr) {
      delete manager_;  // Raw pointer; see definition for rationale.
      manager_ = nullptr;
    }
  }

  //  // TODO: !!!
  //  void ExpectBoolReply(bool expected_value, bool value) {
  //    EXPECT_EQ(expected_value, value);
  //    run_loop_->QuitWhenIdle();
  //  }
  //
  //  // TODO: !!!
  //  base::OnceCallback<void(bool)> ExpectBoolReplyClosure(bool expected_value)
  //  {
  //    return base::BindOnce(&WebRtcEventLogManagerTest::ExpectBoolReply,
  //                          base::Unretained(this), expected_value);
  //  }

  void WaitForReply() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop);  // Allow re-blocking.
  }

  void VoidReply() { run_loop_->QuitWhenIdle(); }

  base::OnceCallback<void(void)> VoidReplyClosure() {
    return base::BindOnce(&WebRtcEventLogManagerTest::VoidReply,
                          base::Unretained(this));
  }

  void BoolReply(bool* output, bool value) {
    *output = value;
    run_loop_->QuitWhenIdle();
  }

  base::OnceCallback<void(bool)> BoolReplyClosure(bool* output) {
    return base::BindOnce(&WebRtcEventLogManagerTest::BoolReply,
                          base::Unretained(this), output);
  }

  bool PeerConnectionAdded(int render_process_id, int lid) {
    bool result;
    manager_->PeerConnectionAdded(render_process_id, lid,
                                  BoolReplyClosure(&result));
    WaitForReply();
    return result;
  }

  bool PeerConnectionRemoved(int render_process_id, int lid) {
    bool result;
    manager_->PeerConnectionRemoved(render_process_id, lid,
                                    BoolReplyClosure(&result));
    WaitForReply();
    return result;
  }

  bool EnableLocalLogging(
      size_t max_size_bytes = WebRtcEventLogManager::kUnlimitedFileSize) {
    bool result;
    manager_->EnableLocalLogging(base_path_, max_size_bytes,
                                 BoolReplyClosure(&result));
    WaitForReply();
    return result;
  }

  bool DisableLocalLogging() {
    bool result;
    manager_->DisableLocalLogging(BoolReplyClosure(&result));
    WaitForReply();
    return result;
  }

  void SetLocalLogsObserver(
      WebRtcEventLogManager::LocalLogsObserver* observer) {
    manager_->SetLocalLogsObserver(observer, VoidReplyClosure());
    WaitForReply();
  }

  //  // TODO: !!!
  //  // With partial binding, we'll get a closure that will write the reply
  //  // into a predefined destination. (Diverging from the style-guide by
  //  putting
  //  // an output parameter first is necessary for partial binding.)
  //  void OnFilePathReply(base::FilePath* out_path, base::FilePath value) {
  //    *out_path = value;
  //    run_loop_->QuitWhenIdle();
  //  }
  //
  //  base::OnceCallback<void(base::FilePath)> FilePathReplyClosure(
  //      base::FilePath* file_path) {
  //    return base::BindOnce(&WebRtcEventLogManagerTest::OnFilePathReply,
  //                          base::Unretained(this), file_path);
  //  }
  //
  //  base::FilePath LocalWebRtcEventLogStart(int render_process_id,
  //                                          int lid,
  //                                          const base::FilePath& base_path,
  //                                          size_t max_file_size) {
  //    base::FilePath file_path;
  //    manager_->LocalWebRtcEventLogStart(render_process_id, lid, base_path,
  //                                       max_file_size,
  //                                       FilePathReplyClosure(&file_path));
  //    WaitForReply();
  //    return file_path;
  //  }
  //
  //  void LocalWebRtcEventLogStop(int render_process_id,
  //                               int lid,
  //                               ExpectedResult expected_result) {
  //    const bool expected_result_bool = static_cast<bool>(expected_result);
  //    manager_->LocalWebRtcEventLogStop(
  //        render_process_id, lid,
  //        ExpectBoolReplyClosure(expected_result_bool));
  //    WaitForReply();
  //  }

  bool OnWebRtcEventLogWrite(int render_process_id,
                             int lid,
                             const std::string& output) {
    bool result;
    manager_->OnWebRtcEventLogWrite(render_process_id, lid, output,
                                    BoolReplyClosure(&result));
    WaitForReply();
    return result;
  }

  void FreezeClockAt(const base::Time::Exploded& frozen_time_exploded) {
    base::Time frozen_time;
    ASSERT_TRUE(
        base::Time::FromLocalExploded(frozen_time_exploded, &frozen_time));
    frozen_clock_.SetNow(frozen_time);
    manager_->InjectClockForTesting(&frozen_clock_);
  }

  // Common default values.
  static constexpr int kRenderProcessId = 23;
  static constexpr int kLocalPeerConnectionId = 478;
  static constexpr size_t kMaxFileSizeBytes = 50000;

  // Testing utilities.
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  std::unique_ptr<base::RunLoop> run_loop_;  // New instance allows reblocking.
  base::SimpleTestClock frozen_clock_;

  // Unit under test. (Cannot be a unique_ptr because of its private ctor/dtor.)
  WebRtcEventLogManager* manager_;

  base::FilePath base_dir_;   // The directory where we'll save log files.
  base::FilePath base_path_;  // base_dir_ +  log files' name prefix.
};

TEST_F(WebRtcEventLogManagerTest, LocalLogPeerConnectionAddedReturnsTrue) {
  EXPECT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogPeerConnectionAddedReturnsFalseIfAlreadyAdded) {
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  EXPECT_FALSE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogPeerConnectionRemovedReturnsTrue) {
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  EXPECT_TRUE(PeerConnectionRemoved(kRenderProcessId, kLocalPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogPeerConnectionRemovedReturnsFalseIfNeverAdded) {
  EXPECT_FALSE(PeerConnectionRemoved(kRenderProcessId, kLocalPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogPeerConnectionRemovedReturnsFalseIfAlreadyRemoved) {
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(PeerConnectionRemoved(kRenderProcessId, kLocalPeerConnectionId));
  EXPECT_FALSE(PeerConnectionRemoved(kRenderProcessId, kLocalPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogEnableLocalLoggingReturnsTrue) {
  EXPECT_TRUE(EnableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogEnableLocalLoggingReturnsFalseIfCalledWhenAlreadyEnabled) {
  ASSERT_TRUE(EnableLocalLogging());
  EXPECT_FALSE(EnableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest, LocalLogDisableLocalLoggingReturnsTrue) {
  ASSERT_TRUE(EnableLocalLogging());
  EXPECT_TRUE(DisableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogDisableLocalLoggingReturnsIfNeverEnabled) {
  EXPECT_FALSE(DisableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogDisableLocalLoggingReturnsIfAlreadyDisabled) {
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(DisableLocalLogging());
  EXPECT_FALSE(DisableLocalLogging());
}

// TODO: !!! Test that disabling logging stops active files.

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogsStartedNotCalledIfLocalLoggingEnabledWithoutPeerConnections) {
  MockLocalLogsObserver observer;
  EXPECT_CALL(observer, OnLocalLogsStarted(_, _)).Times(0);
  EXPECT_CALL(observer, OnLocalLogsStopped(_)).Times(0);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(EnableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogsStartedCalledForPeerConnectionAddedAndLocalLoggingEnabled) {
  MockLocalLogsObserver observer;
  PeerConnectionKey peer_connection(kRenderProcessId, kLocalPeerConnectionId);
  EXPECT_CALL(observer, OnLocalLogsStarted(peer_connection, _)).Times(1);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogsStartedCalledForLocalLoggingEnabledAndPeerConnectionAdded) {
  MockLocalLogsObserver observer;
  PeerConnectionKey peer_connection(kRenderProcessId, kLocalPeerConnectionId);
  EXPECT_CALL(observer, OnLocalLogsStarted(peer_connection, _)).Times(1);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogsStoppedCalledAfterLocalLoggingDisabled) {
  NiceMock<MockLocalLogsObserver> observer;
  PeerConnectionKey peer_connection(kRenderProcessId, kLocalPeerConnectionId);
  EXPECT_CALL(observer, OnLocalLogsStopped(peer_connection)).Times(1);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(DisableLocalLogging());
}

void printme(PeerConnectionKey key) {
  printf("!!!\n");
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogsStoppedCalledAfterPeerConnectionRemoved) {
  NiceMock<MockLocalLogsObserver> observer;
  PeerConnectionKey peer_connection(kRenderProcessId, kLocalPeerConnectionId);
  EXPECT_CALL(observer, OnLocalLogsStopped(peer_connection)).Times(1);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionRemoved(kRenderProcessId, kLocalPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest, RemovedLocalLogsObserverReceivesNoCalls) {
  MockLocalLogsObserver observer;
  EXPECT_CALL(observer, OnLocalLogsStarted(_, _)).Times(0);
  EXPECT_CALL(observer, OnLocalLogsStopped(_)).Times(0);
  SetLocalLogsObserver(&observer);
  SetLocalLogsObserver(nullptr);
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(PeerConnectionRemoved(kRenderProcessId, kLocalPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogCreatesEmptyFileWhenStarted) {
  NiceMock<MockLocalLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  base::Optional<PeerConnectionKey> key;
  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogsStarted(_, _))
      .WillByDefault(Invoke(SaveKeyAndFilePathTo(&key, &file_path)));

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(PeerConnectionRemoved(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(key);
  ASSERT_TRUE(file_path);

  EXPECT_EQ(*key, PeerConnectionKey(kRenderProcessId, kLocalPeerConnectionId));
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, "");
}

TEST_F(WebRtcEventLogManagerTest, LocalLogCreateAndWriteToFile) {
  NiceMock<MockLocalLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  base::Optional<PeerConnectionKey> key;
  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogsStarted(_, _))
      .WillByDefault(Invoke(SaveKeyAndFilePathTo(&key, &file_path)));

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(key);
  ASSERT_TRUE(file_path);

  const std::string log = "To strive, to seek, to find, and not to yield.";
  // TODO: !!! Test OnWebRtcEventLogWrite's return value. (1) for never started,
  // (2) for stopped.
  ASSERT_TRUE(
      OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log));
  ASSERT_TRUE(PeerConnectionRemoved(kRenderProcessId, kLocalPeerConnectionId));

  EXPECT_EQ(*key, PeerConnectionKey(kRenderProcessId, kLocalPeerConnectionId));
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, file_contents);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleWritesToSameFile) {
  NiceMock<MockLocalLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  base::Optional<PeerConnectionKey> key;
  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogsStarted(_, _))
      .WillByDefault(Invoke(SaveKeyAndFilePathTo(&key, &file_path)));

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(key);
  ASSERT_TRUE(file_path);

  const std::string logs[] = {"Old age hath yet his honour and his toil;",
                              "Death closes all: but something ere the end,",
                              "Some work of noble note, may yet be done,",
                              "Not unbecoming men that strove with Gods."};
  for (const std::string& log : logs) {
    ASSERT_TRUE(
        OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log));
  }

  ASSERT_TRUE(PeerConnectionRemoved(kRenderProcessId, kLocalPeerConnectionId));

  EXPECT_EQ(*key, PeerConnectionKey(kRenderProcessId, kLocalPeerConnectionId));
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents,
            std::accumulate(std::begin(logs), std::end(logs), std::string()));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogFileSizeLimitNotExceeded) {
  NiceMock<MockLocalLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  base::Optional<PeerConnectionKey> key;
  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogsStarted(_, _))
      .WillByDefault(Invoke(SaveKeyAndFilePathTo(&key, &file_path)));

  const std::string log = "There lies the port; the vessel puffs her sail:";
  const size_t file_size_limit_bytes = log.length() / 2;

  ASSERT_TRUE(EnableLocalLogging(file_size_limit_bytes));
  ASSERT_TRUE(PeerConnectionAdded(kRenderProcessId, kLocalPeerConnectionId));
  ASSERT_TRUE(key);  // TODO: !!! Assert on the actual key (everywhere).
  ASSERT_TRUE(file_path);

  // A failure is reported, because not everything could be written. The file
  // will also be closed.
  EXPECT_CALL(observer, OnLocalLogsStopped(PeerConnectionKey(
                            kRenderProcessId, kLocalPeerConnectionId)))
      .Times(1);
  ASSERT_FALSE(
      OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log));

  // Additional calls to Write() have no effect.
  // TODO: !!! Separate test?
  ASSERT_FALSE(OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                                     "ignored"));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, log.substr(0, file_size_limit_bytes));
}

#if 0  // TODO: !!!
TEST_F(WebRtcEventLogManagerTest, LocalLogSanityOverUnlimitedFileSizes) {
  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path_,
      WebRtcEventLogManager::kUnlimitedFileSize);
  ASSERT_FALSE(file_path.empty());

  const std::string log1 = "Who let the dogs out?";
  const std::string log2 = "Woof, woof, woof, woof, woof!";
  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log1,
                        ExpectedResult::kSuccess);
  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log2,
                        ExpectedResult::kSuccess);

  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                          ExpectedResult::kSuccess);

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(file_path, &file_contents));
  EXPECT_EQ(file_contents, log1 + log2);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogNoWriteAfterLogStop) {
  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path_, kMaxFileSizeBytes);
  ASSERT_FALSE(file_path.empty());

  const std::string log_before = "log_before_stop";
  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log_before,
                        ExpectedResult::kSuccess);

  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                          ExpectedResult::kSuccess);

  const std::string log_after = "log_after_stop";
  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log_after,
                        ExpectedResult::kFailure);

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(file_path, &file_contents));
  EXPECT_EQ(file_contents, log_before);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogOnlyWritesTheLogsAfterStart) {
  // Calls to Write() before Start() are ignored.
  const std::string log1 = "The lights begin to twinkle from the rocks:";
  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log1,
                        ExpectedResult::kFailure);
  ASSERT_TRUE(base::IsDirectoryEmpty(base_dir_));

  // Calls after Start() have an effect. The calls to Write() from before
  // Start() are not remembered.
  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path_, kMaxFileSizeBytes);
  ASSERT_FALSE(file_path.empty());

  const std::string log2 = "The long day wanes: the slow moon climbs: the deep";
  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log2,
                        ExpectedResult::kSuccess);
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(file_path, &file_contents));
  EXPECT_EQ(file_contents, log2);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogStopBeforeStartHasNoEffect) {
  // Calls to Stop() before Start() are ignored.
  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                          ExpectedResult::kFailure);
  ASSERT_TRUE(base::IsDirectoryEmpty(base_dir_));

  // The Stop() before does not leave any bad state behind. We can still
  // Start() the log, Write() to it and Close() it.
  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path_, kMaxFileSizeBytes);
  ASSERT_FALSE(file_path.empty());

  const std::string log = "To err is canine; to forgive, feline.";
  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log,
                        ExpectedResult::kSuccess);
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(file_path, &file_contents));
  EXPECT_EQ(file_contents, log);
}

// Note: This test also covers the scenario LocalLogExistingFilesNotOverwritten,
// which is therefore not explicitly tested.
TEST_F(WebRtcEventLogManagerTest, LocalLogRestartedLogCreatesNewFile) {
  const std::vector<std::string> logs = {"<setup>", "<punchline>", "encore"};
  std::vector<base::FilePath> file_paths;
  for (size_t i = 0; i < logs.size(); i++) {
    file_paths.push_back(
        LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                                 base_path_, kMaxFileSizeBytes));
    ASSERT_FALSE(file_paths.back().empty());

    OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, logs[i],
                          ExpectedResult::kSuccess);

    LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                            ExpectedResult::kSuccess);
  }

  for (size_t i = 0; i < logs.size(); i++) {
    std::string file_contents;
    ASSERT_TRUE(base::ReadFileToString(file_paths[i], &file_contents));
    EXPECT_EQ(file_contents, logs[i]);
  }
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleActiveFiles) {
  const std::vector<PeerConnectionKey> keys = {{1, 2}, {2, 1}, {3, 4},
                                               {4, 3}, {5, 5}, {6, 7}};
  std::vector<std::string> logs;
  std::vector<base::FilePath> file_paths;

  for (const auto& key : keys) {
    file_paths.push_back(LocalWebRtcEventLogStart(
        key.render_process_id, key.lid, base_path_, kMaxFileSizeBytes));
    ASSERT_FALSE(file_paths.back().empty());
  }

  for (size_t i = 0; i < keys.size(); i++) {
    logs.emplace_back(std::to_string(kRenderProcessId) +
                      std::to_string(kLocalPeerConnectionId));
    OnWebRtcEventLogWrite(keys[i].render_process_id, keys[i].lid, logs[i],
                          ExpectedResult::kSuccess);
  }

  for (size_t i = 0; i < keys.size(); i++) {
    LocalWebRtcEventLogStop(keys[i].render_process_id, keys[i].lid,
                            ExpectedResult::kSuccess);
  }

  for (size_t i = 0; i < keys.size(); i++) {
    std::string file_contents;
    ASSERT_TRUE(base::ReadFileToString(file_paths[i], &file_contents));
    EXPECT_EQ(file_contents, logs[i]);
  }
}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityDestructorWithActiveFile) {
  // We don't actually test that the file was closed, because this behavior
  // is not supported - WebRtcEventLogManager's destructor may never be called,
  // except for in unit-tests.
  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path_, kMaxFileSizeBytes);
  ASSERT_FALSE(file_path.empty());
  DestroyUnitUnderTest();  // Explicitly show destruction does not crash.
}

// TODO(eladalon): Re-enable after handling https://crbug.com/786374
TEST_F(WebRtcEventLogManagerTest, DISABLED_LocalLogIllegalPath) {
  const base::FilePath illegal_path(FILE_PATH_LITERAL(":!@#$%|`^&*\\/"));
  const base::FilePath file_path =
      LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                               illegal_path, kMaxFileSizeBytes);
  EXPECT_TRUE(file_path.empty());
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));
}

#if defined(OS_POSIX)
TEST_F(WebRtcEventLogManagerTest, LocalLogLegalPathWithoutPermissionsSanity) {
  // Remove writing permissions from the entire directory.
  int permissions;
  ASSERT_TRUE(base::GetPosixFilePermissions(base_dir_, &permissions));
  constexpr int write_permissions = base::FILE_PERMISSION_WRITE_BY_USER |
                                    base::FILE_PERMISSION_WRITE_BY_GROUP |
                                    base::FILE_PERMISSION_WRITE_BY_OTHERS;
  permissions &= ~write_permissions;
  ASSERT_TRUE(base::SetPosixFilePermissions(base_dir_, permissions));

  // Start() has no effect (but is handled gracefully).
  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path_, kMaxFileSizeBytes);
  EXPECT_TRUE(file_path.empty());
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));

  // Write() has no effect (but is handled gracefully).
  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId,
                        "Why did the chicken cross the road?",
                        ExpectedResult::kFailure);
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));

  // Stop() has no effect (but is handled gracefully).
  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                          ExpectedResult::kFailure);
  EXPECT_TRUE(base::IsDirectoryEmpty(base_dir_));
}
#endif

TEST_F(WebRtcEventLogManagerTest, LocalLogEmptyStringHandledGracefully) {
  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path_, kMaxFileSizeBytes);
  ASSERT_FALSE(file_path.empty());

  // By writing a log after the empty string, we show that no odd behavior is
  // encountered, such as closing the file (an actual bug from WebRTC).
  const std::vector<std::string> logs = {"<setup>", "", "encore"};
  for (const auto& log : logs) {
    OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, log,
                          ExpectedResult::kSuccess);
  }

  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                          ExpectedResult::kSuccess);

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(file_path, &file_contents));
  EXPECT_EQ(file_contents,
            std::accumulate(std::begin(logs), std::end(logs), std::string()));
}

// The logs would typically be binary. However, the other tests only cover ASCII
// characters, for readability. This test shows that this is not a problem.
TEST_F(WebRtcEventLogManagerTest, LocalLogAllPossibleCharacters) {
  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path_, kMaxFileSizeBytes);
  ASSERT_FALSE(file_path.empty());

  std::string all_chars;
  for (size_t i = 0; i < 256; i++) {
    all_chars += static_cast<uint8_t>(i);
  }

  OnWebRtcEventLogWrite(kRenderProcessId, kLocalPeerConnectionId, all_chars,
                        ExpectedResult::kSuccess);

  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                          ExpectedResult::kSuccess);

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(file_path, &file_contents));
  EXPECT_EQ(file_contents, all_chars);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogFilenameMatchesExpectedFormat) {
  using StringType = base::FilePath::StringType;

  const base::Time::Exploded frozen_time_exploded{
      2017,  // Four digit year "2007"
      9,     // 1-based month (values 1 = January, etc.)
      3,     // 0-based day of week (0 = Sunday, etc.)
      6,     // 1-based day of month (1-31)
      10,    // Hour within the current day (0-23)
      43,    // Minute within the current hour (0-59)
      29,    // Second within the current minute.
      0      // Milliseconds within the current second (0-999)
  };
  ASSERT_TRUE(frozen_time_exploded.HasValidValues());
  FreezeClockAt(frozen_time_exploded);

  const StringType user_defined = FILE_PATH_LITERAL("user_defined");
  const base::FilePath base_path = base_dir_.Append(user_defined);

  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path, kMaxFileSizeBytes);

  // [user_defined]_[date]_[time]_[pid]_[lid].log
  const StringType date = FILE_PATH_LITERAL("20170906");
  const StringType time = FILE_PATH_LITERAL("1043");
  base::FilePath expected_path = base_path;
  expected_path = base_path.InsertBeforeExtension(
      FILE_PATH_LITERAL("_") + date + FILE_PATH_LITERAL("_") + time +
      FILE_PATH_LITERAL("_") + IntToStringType(kRenderProcessId) +
      FILE_PATH_LITERAL("_") + IntToStringType(kLocalPeerConnectionId));
  expected_path = expected_path.AddExtension(FILE_PATH_LITERAL("log"));

  EXPECT_EQ(file_path, expected_path);
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogFilenameMatchesExpectedFormatRepeatedFilename) {
  using StringType = base::FilePath::StringType;

  const base::Time::Exploded frozen_time_exploded{
      2017,  // Four digit year "2007"
      9,     // 1-based month (values 1 = January, etc.)
      3,     // 0-based day of week (0 = Sunday, etc.)
      6,     // 1-based day of month (1-31)
      10,    // Hour within the current day (0-23)
      43,    // Minute within the current hour (0-59)
      29,    // Second within the current minute.
      0      // Milliseconds within the current second (0-999)
  };
  ASSERT_TRUE(frozen_time_exploded.HasValidValues());
  FreezeClockAt(frozen_time_exploded);

  const StringType user_defined_portion =
      FILE_PATH_LITERAL("user_defined_portion");
  const base::FilePath base_path = base_dir_.Append(user_defined_portion);

  const base::FilePath file_path_1 = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path, kMaxFileSizeBytes);

  // [user_defined]_[date]_[time]_[pid]_[lid].log
  const StringType date = FILE_PATH_LITERAL("20170906");
  const StringType time = FILE_PATH_LITERAL("1043");
  base::FilePath expected_path_1 = base_path;
  expected_path_1 = base_path.InsertBeforeExtension(
      FILE_PATH_LITERAL("_") + date + FILE_PATH_LITERAL("_") + time +
      FILE_PATH_LITERAL("_") + IntToStringType(kRenderProcessId) +
      FILE_PATH_LITERAL("_") + IntToStringType(kLocalPeerConnectionId));
  expected_path_1 = expected_path_1.AddExtension(FILE_PATH_LITERAL("log"));

  ASSERT_EQ(file_path_1, expected_path_1);

  LocalWebRtcEventLogStop(kRenderProcessId, kLocalPeerConnectionId,
                          ExpectedResult::kSuccess);

  const base::FilePath file_path_2 = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path, kMaxFileSizeBytes);

  const base::FilePath expected_path_2 =
      expected_path_1.InsertBeforeExtension(FILE_PATH_LITERAL(" (1)"));

  // Focus of the test - starting the same log again produces a new file,
  // with an expected new filename.
  ASSERT_EQ(file_path_2, expected_path_2);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMayNotBeStartedTwice) {
  const base::FilePath file_path = LocalWebRtcEventLogStart(
      kRenderProcessId, kLocalPeerConnectionId, base_path_, kMaxFileSizeBytes);
  ASSERT_FALSE(file_path.empty());
  // Known issue - this always passes (whether LocalWebRtcEventLogStart crashes
  // or not). http://crbug.com/787809
  EXPECT_DEATH_IF_SUPPORTED(
      LocalWebRtcEventLogStart(kRenderProcessId, kLocalPeerConnectionId,
                               base_path_, kMaxFileSizeBytes),
      "");
}
#endif  // TODO: !!!

}  // namespace content
