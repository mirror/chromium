// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/gtest_util.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "content/browser/webrtc/webrtc_remote_event_log_manager.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// TODO(eladalon): Add unit tests for incognito mode. https://crbug.com/775415

#if defined(OS_WIN)
#define IntToStringType base::IntToString16
#else
#define IntToStringType base::IntToString
#endif

namespace content {

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::StrictMock;

using PeerConnectionKey = WebRtcEventLogPeerConnectionKey;

class WebRtcEventLogManagerTest : public ::testing::Test {
 public:
  WebRtcEventLogManagerTest()
      : run_loop_(std::make_unique<base::RunLoop>()),
        manager_(WebRtcEventLogManager::CreateSingletonInstance()) {
    EXPECT_TRUE(base::CreateNewTempDirectory(FILE_PATH_LITERAL(""),
                                             &local_logs_base_dir_));
    if (local_logs_base_dir_.empty()) {
      EXPECT_TRUE(false);
      return;
    }
    local_logs_base_path_ =
        local_logs_base_dir_.Append(FILE_PATH_LITERAL("webrtc_event_log"));
  }

  void SetUp() override {
    browser_context_ = CreateBrowserContext();
    rph_ = std::make_unique<MockRenderProcessHost>(browser_context_.get());
  }

  ~WebRtcEventLogManagerTest() override {
    DestroyUnitUnderTest();
    if (!local_logs_base_dir_.empty()) {
      EXPECT_TRUE(base::DeleteFile(local_logs_base_dir_, true));
    }
    // Guard against unexpected state changes.
    EXPECT_TRUE(webrtc_state_change_instructions_.empty());
  }

  void DestroyUnitUnderTest() {
    if (manager_ != nullptr) {
      SetLocalLogsObserver(nullptr);
      delete manager_;  // Raw pointer; see definition for rationale.
      manager_ = nullptr;
    }
  }

  void WaitForReply() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop);  // Allow re-blocking.
  }

  void VoidReply() { run_loop_->QuitWhenIdle(); }

  base::OnceClosure VoidReplyClosure() {
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

  void BoolPairReply(std::pair<bool, bool>* output,
                     std::pair<bool, bool> value) {
    *output = value;
    run_loop_->QuitWhenIdle();
  }

  base::OnceCallback<void(std::pair<bool, bool>)> BoolPairReplyClosure(
      std::pair<bool, bool>* output) {
    return base::BindOnce(&WebRtcEventLogManagerTest::BoolPairReply,
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
      size_t max_size_bytes = kWebRtcEventLogManagerUnlimitedFileSize) {
    return EnableLocalLogging(local_logs_base_path_, max_size_bytes);
  }

  bool EnableLocalLogging(
      base::FilePath local_logs_base_path,
      size_t max_size_bytes = kWebRtcEventLogManagerUnlimitedFileSize) {
    bool result;
    manager_->EnableLocalLogging(local_logs_base_path, max_size_bytes,
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

  bool StartRemoteLogging(int render_process_id,
                          int lid,
                          size_t max_size_bytes = kArbitraryVeryLargeFileSize) {
    bool result;
    manager_->StartRemoteLogging(render_process_id, lid, max_size_bytes,
                                 BoolReplyClosure(&result));
    WaitForReply();
    return result;
  }

  void SetLocalLogsObserver(WebRtcLocalEventLogsObserver* observer) {
    manager_->SetLocalLogsObserver(observer, VoidReplyClosure());
    WaitForReply();
  }

  void SetRemoteLogsObserver(WebRtcRemoteEventLogsObserver* observer) {
    manager_->SetRemoteLogsObserver(observer, VoidReplyClosure());
    WaitForReply();
  }

  std::pair<bool, bool> OnWebRtcEventLogWrite(int render_process_id,
                                              int lid,
                                              const std::string& output) {
    std::pair<bool, bool> result;
    manager_->OnWebRtcEventLogWrite(render_process_id, lid, output,
                                    BoolPairReplyClosure(&result));
    WaitForReply();
    return result;
  }

  void FreezeClockAt(const base::Time::Exploded& frozen_time_exploded) {
    base::Time frozen_time;
    ASSERT_TRUE(
        base::Time::FromLocalExploded(frozen_time_exploded, &frozen_time));
    frozen_clock_.SetNow(frozen_time);
    manager_->SetClockForTesting(&frozen_clock_);
  }

  void SetWebRtcEventLoggingState(PeerConnectionKey key,
                                  bool event_logging_enabled) {
    webrtc_state_change_instructions_.emplace(key, event_logging_enabled);
  }

  void ExpectWebRtcStateChangeInstruction(int render_process_id,
                                          int lid,
                                          bool enabled) {
    ASSERT_FALSE(webrtc_state_change_instructions_.empty());
    auto& instruction = webrtc_state_change_instructions_.front();
    EXPECT_EQ(instruction.key, PeerConnectionKey(render_process_id, lid));
    EXPECT_EQ(instruction.enabled, enabled);
    webrtc_state_change_instructions_.pop();
  }

  void SetPeerConnectionTrackerProxyForTesting(
      std::unique_ptr<WebRtcEventLogManager::PeerConnectionTrackerProxy>
          pc_tracker_proxy) {
    manager_->SetPeerConnectionTrackerProxyForTesting(
        std::move(pc_tracker_proxy));
  }

  // Creates a browser context. Blocks on the unit under test's task runner,
  // so that we won't proceed with the test (e.g. check that files were created)
  // before it has finished processing this even (which is signaled to it
  // from BrowserContext::Initialize).
  std::unique_ptr<TestBrowserContext> CreateBrowserContext(
      base::FilePath local_logs_base_path = base::FilePath()) {
    auto browser_context =
        std::make_unique<TestBrowserContext>(local_logs_base_path);

    base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);
    manager_->GetTaskRunnerForTesting()->PostTask(
        FROM_HERE,
        base::BindOnce([](base::WaitableEvent* event) { event->Signal(); },
                       &event));
    event.Wait();

    return browser_context;
  }

  base::FilePath GetLogsDirectoryPath(const BrowserContext* browser_context) {
    return WebRtcRemoteEventLogManager::GetLogsDirectoryPath(browser_context);
  }

  static const size_t kArbitraryVeryLargeFileSize = 1000 * 1000 * 1000;

  // Testing utilities.
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  std::unique_ptr<base::RunLoop> run_loop_;  // New instance allows reblocking.
  base::SimpleTestClock frozen_clock_;

  // Unit under test. (Cannot be a unique_ptr because of its private ctor/dtor.)
  WebRtcEventLogManager* manager_;

  std::unique_ptr<TestBrowserContext> browser_context_;
  std::unique_ptr<MockRenderProcessHost> rph_;

  // The directory where we'll save local log files.
  base::FilePath local_logs_base_dir_;
  // local_logs_base_dir_ +  log files' name prefix.
  base::FilePath local_logs_base_path_;

  // WebRtcEventLogManager instructs WebRTC, via PeerConnectionTracker, to
  // only send WebRTC messages for certain peer connections. Some tests make
  // sure that this is done correctly, by waiting for these notifications, then
  // testing them.
  struct WebRtcStateChangeInstruction {
    WebRtcStateChangeInstruction(PeerConnectionKey key, bool enabled)
        : key(key), enabled(enabled) {}
    PeerConnectionKey key;
    bool enabled;
  };
  std::queue<WebRtcStateChangeInstruction> webrtc_state_change_instructions_;
};

namespace {

// Common default/arbitrary values.
static constexpr int kPeerConnectionId = 478;

class MockWebRtcLocalEventLogsObserver : public WebRtcLocalEventLogsObserver {
 public:
  ~MockWebRtcLocalEventLogsObserver() override = default;
  MOCK_METHOD2(OnLocalLogStarted,
               void(PeerConnectionKey, const base::FilePath&));
  MOCK_METHOD1(OnLocalLogStopped, void(PeerConnectionKey));
};

class MockWebRtcRemoteEventLogsObserver : public WebRtcRemoteEventLogsObserver {
 public:
  ~MockWebRtcRemoteEventLogsObserver() override = default;
  MOCK_METHOD2(OnRemoteLogStarted,
               void(PeerConnectionKey, const base::FilePath&));
  MOCK_METHOD1(OnRemoteLogStopped, void(PeerConnectionKey));
};

auto SaveFilePathTo(base::Optional<base::FilePath>* output) {
  return [output](PeerConnectionKey ignored_key, base::FilePath file_path) {
    *output = file_path;
  };
}

auto SaveKeyAndFilePathTo(base::Optional<PeerConnectionKey>* key_output,
                          base::Optional<base::FilePath>* file_path_output) {
  return [key_output, file_path_output](PeerConnectionKey key,
                                        base::FilePath file_path) {
    *key_output = key;
    *file_path_output = file_path;
  };
}

class PeerConnectionTrackerProxyForTesting
    : public WebRtcEventLogManager::PeerConnectionTrackerProxy {
 public:
  explicit PeerConnectionTrackerProxyForTesting(WebRtcEventLogManagerTest* test)
      : test_(test) {}
  ~PeerConnectionTrackerProxyForTesting() override = default;

  void SetWebRtcEventLoggingState(WebRtcEventLogPeerConnectionKey key,
                                  bool event_logging_enabled) override {
    test_->SetWebRtcEventLoggingState(key, event_logging_enabled);
  }

 private:
  WebRtcEventLogManagerTest* const test_;
};

#if defined(OS_POSIX) && !defined(OS_FUCHSIA)
void RemoveWritePermissionsFromDirectory(const base::FilePath& path) {
  int permissions;
  ASSERT_TRUE(base::GetPosixFilePermissions(path, &permissions));
  constexpr int write_permissions = base::FILE_PERMISSION_WRITE_BY_USER |
                                    base::FILE_PERMISSION_WRITE_BY_GROUP |
                                    base::FILE_PERMISSION_WRITE_BY_OTHERS;
  permissions &= ~write_permissions;
  ASSERT_TRUE(base::SetPosixFilePermissions(path, permissions));
}
#endif  // defined(OS_POSIX) && !defined(OS_FUCHSIA)

}  // namespace

TEST_F(WebRtcEventLogManagerTest, LocalLogPeerConnectionAddedReturnsTrue) {
  EXPECT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogPeerConnectionAddedReturnsFalseIfAlreadyAdded) {
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  EXPECT_FALSE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogPeerConnectionRemovedReturnsTrue) {
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  EXPECT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogPeerConnectionRemovedReturnsFalseIfNeverAdded) {
  EXPECT_FALSE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogPeerConnectionRemovedReturnsFalseIfAlreadyRemoved) {
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
  EXPECT_FALSE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
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

TEST_F(WebRtcEventLogManagerTest,
       OnWebRtcEventLogWriteReturnsFalseAndFalseWhenAllLoggingDisabled) {
  // Note that EnableLocalLogging() and StartRemoteLogging() weren't called.
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  EXPECT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, "log"),
            std::make_pair(false, false));
}

TEST_F(WebRtcEventLogManagerTest,
       OnWebRtcEventLogWriteReturnsFalseAndFalseForUnknownPeerConnection) {
  ASSERT_TRUE(EnableLocalLogging());
  // Note that PeerConnectionAdded() wasn't called.
  EXPECT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, "log"),
            std::make_pair(false, false));
}

TEST_F(WebRtcEventLogManagerTest,
       OnWebRtcEventLogWriteReturnsLocalTrueWhenPcKnownAndLocalLoggingOn) {
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  EXPECT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, "log"),
            std::make_pair(true, false));
}

TEST_F(WebRtcEventLogManagerTest,
       OnWebRtcEventLogWriteReturnsRemoteTrueWhenPcKnownAndRemoteLogging) {
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  EXPECT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, "log"),
            std::make_pair(false, true));
}

TEST_F(WebRtcEventLogManagerTest,
       OnWebRtcEventLogWriteReturnsTrueAndTrueeWhenAllLoggingEnabled) {
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  EXPECT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, "log"),
            std::make_pair(true, true));
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogStartedNotCalledIfLocalLoggingEnabledWithoutPeerConnections) {
  MockWebRtcLocalEventLogsObserver observer;
  EXPECT_CALL(observer, OnLocalLogStarted(_, _)).Times(0);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(EnableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogStoppedNotCalledIfLocalLoggingDisabledWithoutPeerConnections) {
  MockWebRtcLocalEventLogsObserver observer;
  EXPECT_CALL(observer, OnLocalLogStopped(_)).Times(0);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(DisableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogStartedCalledForPeerConnectionAddedAndLocalLoggingEnabled) {
  MockWebRtcLocalEventLogsObserver observer;
  PeerConnectionKey peer_connection(rph_->GetID(), kPeerConnectionId);
  EXPECT_CALL(observer, OnLocalLogStarted(peer_connection, _)).Times(1);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogStartedCalledForLocalLoggingEnabledAndPeerConnectionAdded) {
  MockWebRtcLocalEventLogsObserver observer;
  PeerConnectionKey peer_connection(rph_->GetID(), kPeerConnectionId);
  EXPECT_CALL(observer, OnLocalLogStarted(peer_connection, _)).Times(1);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogStoppedCalledAfterLocalLoggingDisabled) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  PeerConnectionKey peer_connection(rph_->GetID(), kPeerConnectionId);
  EXPECT_CALL(observer, OnLocalLogStopped(peer_connection)).Times(1);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(DisableLocalLogging());
}

TEST_F(WebRtcEventLogManagerTest,
       OnLocalLogStoppedCalledAfterPeerConnectionRemoved) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  PeerConnectionKey peer_connection(rph_->GetID(), kPeerConnectionId);
  EXPECT_CALL(observer, OnLocalLogStopped(peer_connection)).Times(1);
  SetLocalLogsObserver(&observer);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest, RemovedLocalLogsObserverReceivesNoCalls) {
  StrictMock<MockWebRtcLocalEventLogsObserver> observer;
  EXPECT_CALL(observer, OnLocalLogStarted(_, _)).Times(0);
  EXPECT_CALL(observer, OnLocalLogStopped(_)).Times(0);
  SetLocalLogsObserver(&observer);
  SetLocalLogsObserver(nullptr);
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogCreatesEmptyFileWhenStarted) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);
  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  // Make sure the file would be closed, so that we could safely read it.
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, "");
}

TEST_F(WebRtcEventLogManagerTest, LocalLogCreateAndWriteToFile) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  const std::string log = "To strive, to seek, to find, and not to yield.";
  ASSERT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, log),
            std::make_pair(true, false));

  // Make sure the file would be closed, so that we could safely read it.
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, log);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleWritesToSameFile) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  const std::string logs[] = {"Old age hath yet his honour and his toil;",
                              "Death closes all: but something ere the end,",
                              "Some work of noble note, may yet be done,",
                              "Not unbecoming men that strove with Gods."};
  for (const std::string& log : logs) {
    ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log),
              std::make_pair(true, false));
  }

  // Make sure the file would be closed, so that we could safely read it.
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents,
            std::accumulate(std::begin(logs), std::end(logs), std::string()));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogFileSizeLimitNotExceeded) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

  const std::string log = "There lies the port; the vessel puffs her sail:";
  const size_t file_size_limit_bytes = log.length() / 2;

  ASSERT_TRUE(EnableLocalLogging(file_size_limit_bytes));
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  // A failure is reported, because not everything could be written. The file
  // will also be closed.
  const auto pc = PeerConnectionKey(key.render_process_id, key.lid);
  EXPECT_CALL(observer, OnLocalLogStopped(pc)).Times(1);
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log),
            std::make_pair(false, false));

  // Additional calls to Write() have no effect.
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, "ignored"),
            std::make_pair(false, false));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, log.substr(0, file_size_limit_bytes));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityOverUnlimitedFileSizes) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

  ASSERT_TRUE(EnableLocalLogging(kWebRtcEventLogManagerUnlimitedFileSize));
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  const std::string log1 = "Who let the dogs out?";
  const std::string log2 = "Woof, woof, woof, woof, woof!";
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log1),
            std::make_pair(true, false));
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log2),
            std::make_pair(true, false));

  // Make sure the file would be closed, so that we could safely read it.
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, log1 + log2);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogNoWriteAfterLogStopped) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  const std::string log_before = "log_before_stop";
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log_before),
            std::make_pair(true, false));
  EXPECT_CALL(observer, OnLocalLogStopped(key)).Times(1);
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));

  const std::string log_after = "log_after_stop";
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log_after),
            std::make_pair(false, false));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, log_before);
}

TEST_F(WebRtcEventLogManagerTest, LocalLogOnlyWritesTheLogsAfterStarted) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  // Calls to Write() before the log was started are ignored.
  EXPECT_CALL(observer, OnLocalLogStarted(_, _)).Times(0);
  const std::string log1 = "The lights begin to twinkle from the rocks:";
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log1),
            std::make_pair(false, false));
  ASSERT_TRUE(base::IsDirectoryEmpty(local_logs_base_dir_));

  base::Optional<base::FilePath> file_path;
  EXPECT_CALL(observer, OnLocalLogStarted(key, _))
      .Times(1)
      .WillOnce(Invoke(SaveFilePathTo(&file_path)));

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  // Calls after the log started have an effect. The calls to Write() from
  // before the log started are not remembered.
  const std::string log2 = "The long day wanes: the slow moon climbs: the deep";
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log2),
            std::make_pair(true, false));

  // Make sure the file would be closed, so that we could safely read it.
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, log2);
}

// Note: This test also covers the scenario LocalLogExistingFilesNotOverwritten,
// which is therefore not explicitly tested.
TEST_F(WebRtcEventLogManagerTest, LocalLoggingRestartCreatesNewFile) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const std::vector<std::string> logs = {"<setup>", "<punchline>", "<encore>"};
  std::vector<base::Optional<PeerConnectionKey>> keys(logs.size());
  std::vector<base::Optional<base::FilePath>> file_paths(logs.size());

  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));

  for (size_t i = 0; i < logs.size(); i++) {
    ON_CALL(observer, OnLocalLogStarted(_, _))
        .WillByDefault(Invoke(SaveKeyAndFilePathTo(&keys[i], &file_paths[i])));
    ASSERT_TRUE(EnableLocalLogging());
    ASSERT_TRUE(keys[i]);
    ASSERT_EQ(*keys[i], PeerConnectionKey(rph_->GetID(), kPeerConnectionId));
    ASSERT_TRUE(file_paths[i]);
    ASSERT_FALSE(file_paths[i]->empty());
    ASSERT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, logs[i]),
              std::make_pair(true, false));
    ASSERT_TRUE(DisableLocalLogging());
  }

  for (size_t i = 0; i < logs.size(); i++) {
    std::string file_contents;
    ASSERT_TRUE(base::ReadFileToString(*file_paths[i], &file_contents));
    EXPECT_EQ(file_contents, logs[i]);
  }
}

TEST_F(WebRtcEventLogManagerTest, LocalLogMultipleActiveFiles) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  ASSERT_TRUE(EnableLocalLogging());

  std::list<MockRenderProcessHost> rphs;
  for (size_t i = 0; i < 3; i++) {
    rphs.emplace_back(browser_context_.get());
  };

  std::vector<PeerConnectionKey> keys;
  for (auto& rph : rphs) {
    keys.emplace_back(rph.GetID(), kPeerConnectionId);
  }

  std::vector<base::Optional<base::FilePath>> file_paths(keys.size());
  for (size_t i = 0; i < keys.size(); i++) {
    ON_CALL(observer, OnLocalLogStarted(keys[i], _))
        .WillByDefault(Invoke(SaveFilePathTo(&file_paths[i])));
    ASSERT_TRUE(PeerConnectionAdded(keys[i].render_process_id, keys[i].lid));
    ASSERT_TRUE(file_paths[i]);
    ASSERT_FALSE(file_paths[i]->empty());
  }

  std::vector<std::string> logs;
  for (size_t i = 0; i < keys.size(); i++) {
    logs.emplace_back(std::to_string(rph_->GetID()) +
                      std::to_string(kPeerConnectionId));
    ASSERT_EQ(
        OnWebRtcEventLogWrite(keys[i].render_process_id, keys[i].lid, logs[i]),
        std::make_pair(true, false));
  }

  // Make sure the file woulds be closed, so that we could safely read them.
  ASSERT_TRUE(DisableLocalLogging());

  for (size_t i = 0; i < keys.size(); i++) {
    std::string file_contents;
    ASSERT_TRUE(base::ReadFileToString(*file_paths[i], &file_contents));
    EXPECT_EQ(file_contents, logs[i]);
  }
}

TEST_F(WebRtcEventLogManagerTest, LocalLogLimitActiveLocalLogFiles) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  ASSERT_TRUE(EnableLocalLogging());

  const int kMaxLocalLogFiles =
      static_cast<int>(kMaxNumberLocalWebRtcEventLogFiles);
  int i;
  for (i = 0; i < kMaxLocalLogFiles; i++) {
    EXPECT_CALL(observer, OnLocalLogStarted(PeerConnectionKey(i, i), _))
        .Times(1);
    ASSERT_TRUE(PeerConnectionAdded(i, i));
  }

  EXPECT_CALL(observer, OnLocalLogStarted(_, _)).Times(0);
  ASSERT_TRUE(PeerConnectionAdded(i, i));
}

// When a log reaches its maximum size limit, it is closed, and no longer
// counted towards the limit.
TEST_F(WebRtcEventLogManagerTest, LocalLogFilledLogNotCountedTowardsLogsLimit) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const std::string log = "very_short_log";
  ASSERT_TRUE(EnableLocalLogging(log.size()));

  const int kMaxLocalLogFiles =
      static_cast<int>(kMaxNumberLocalWebRtcEventLogFiles);
  int i;
  for (i = 0; i < kMaxLocalLogFiles - 1; i++) {
    EXPECT_CALL(observer,
                OnLocalLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
        .Times(1);
    ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
  }

  // Last allowed log (we've started kMaxLocalLogFiles - 1 so far).
  EXPECT_CALL(observer,
              OnLocalLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
      .Times(1);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
  // By writing to it, we fill it and end up closing it, allowing an additional
  // log to be written.
  EXPECT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), i, log),
            std::make_pair(true, false));

  // We now have room for one additional log.
  ++i;
  EXPECT_CALL(observer,
              OnLocalLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
      .Times(1);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogForRemovedPeerConnectionNotCountedTowardsLogsLimit) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  ASSERT_TRUE(EnableLocalLogging());

  const int kMaxLocalLogFiles =
      static_cast<int>(kMaxNumberLocalWebRtcEventLogFiles);
  int i;
  for (i = 0; i < kMaxLocalLogFiles - 1; i++) {
    EXPECT_CALL(observer,
                OnLocalLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
        .Times(1);
    ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
  }

  // Last allowed log (we've started kMaxLocalLogFiles - 1 so far).
  EXPECT_CALL(observer,
              OnLocalLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
      .Times(1);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));

  // By removing this peer connection, we allow an additional log.
  EXPECT_CALL(observer, OnLocalLogStopped(PeerConnectionKey(rph_->GetID(), i)))
      .Times(1);
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), i));

  // We now have room for one additional log.
  ++i;
  EXPECT_CALL(observer,
              OnLocalLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
      .Times(1);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogSanityDestructorWithActiveFile) {
  // We don't actually test that the file was closed, because this behavior
  // is not supported - WebRtcEventLogManager's destructor may never be called,
  // except for in unit tests.
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  DestroyUnitUnderTest();  // Explicitly show destruction does not crash.
}

TEST_F(WebRtcEventLogManagerTest, LocalLogIllegalPath) {
  StrictMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  // Since the log file won't be properly opened, these will not be called.
  EXPECT_CALL(observer, OnLocalLogStarted(_, _)).Times(0);
  EXPECT_CALL(observer, OnLocalLogStopped(_)).Times(0);

  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));

  // See the documentation of the function for why |true| is expected despite
  // the path being illegal.
  const base::FilePath illegal_path(FILE_PATH_LITERAL(":!@#$%|`^&*\\/"));
  EXPECT_TRUE(EnableLocalLogging(illegal_path));

  EXPECT_TRUE(base::IsDirectoryEmpty(local_logs_base_dir_));
}

#if defined(OS_POSIX) && !defined(OS_FUCHSIA)
TEST_F(WebRtcEventLogManagerTest, LocalLogLegalPathWithoutPermissionsSanity) {
  RemoveWritePermissionsFromDirectory(local_logs_base_dir_);

  StrictMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  // Since the log file won't be properly opened, these will not be called.
  EXPECT_CALL(observer, OnLocalLogStarted(_, _)).Times(0);
  EXPECT_CALL(observer, OnLocalLogStopped(_)).Times(0);

  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));

  // See the documentation of the function for why |true| is expected despite
  // the path being illegal.
  EXPECT_TRUE(EnableLocalLogging(local_logs_base_path_));

  EXPECT_TRUE(base::IsDirectoryEmpty(local_logs_base_dir_));

  // Write() has no effect (but is handled gracefully).
  EXPECT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId,
                                  "Why did the chicken cross the road?"),
            std::make_pair(false, false));
  EXPECT_TRUE(base::IsDirectoryEmpty(local_logs_base_dir_));

  // Logging was enabled, even if it had no effect because of the lacking
  // permissions; therefore, the operation of disabling it makes sense.
  EXPECT_TRUE(DisableLocalLogging());
  EXPECT_TRUE(base::IsDirectoryEmpty(local_logs_base_dir_));
}
#endif  // defined(OS_POSIX) && !defined(OS_FUCHSIA)

TEST_F(WebRtcEventLogManagerTest, LocalLogEmptyStringHandledGracefully) {
  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  // By writing a log after the empty string, we show that no odd behavior is
  // encountered, such as closing the file (an actual bug from WebRTC).
  const std::vector<std::string> logs = {"<setup>", "", "<encore>"};

  base::Optional<base::FilePath> file_path;

  ON_CALL(observer, OnLocalLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  for (size_t i = 0; i < logs.size(); i++) {
    ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, logs[i]),
              std::make_pair(true, false));
  }
  ASSERT_TRUE(DisableLocalLogging());

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents,
            std::accumulate(std::begin(logs), std::end(logs), std::string()));
}

TEST_F(WebRtcEventLogManagerTest, LocalLogFilenameMatchesExpectedFormat) {
  using StringType = base::FilePath::StringType;

  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnLocalLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

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
  const base::FilePath local_logs_base_path =
      local_logs_base_dir_.Append(user_defined);

  ASSERT_TRUE(EnableLocalLogging(local_logs_base_path));
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  // [user_defined]_[date]_[time]_[pid]_[lid].log
  const StringType date = FILE_PATH_LITERAL("20170906");
  const StringType time = FILE_PATH_LITERAL("1043");
  base::FilePath expected_path = local_logs_base_path;
  expected_path = local_logs_base_path.InsertBeforeExtension(
      FILE_PATH_LITERAL("_") + date + FILE_PATH_LITERAL("_") + time +
      FILE_PATH_LITERAL("_") + IntToStringType(rph_->GetID()) +
      FILE_PATH_LITERAL("_") + IntToStringType(kPeerConnectionId));
  expected_path = expected_path.AddExtension(FILE_PATH_LITERAL("log"));

  EXPECT_EQ(file_path, expected_path);
}

TEST_F(WebRtcEventLogManagerTest,
       LocalLogFilenameMatchesExpectedFormatRepeatedFilename) {
  using StringType = base::FilePath::StringType;

  NiceMock<MockWebRtcLocalEventLogsObserver> observer;
  SetLocalLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  base::Optional<base::FilePath> file_path_1;
  base::Optional<base::FilePath> file_path_2;
  EXPECT_CALL(observer, OnLocalLogStarted(key, _))
      .WillOnce(Invoke(SaveFilePathTo(&file_path_1)))
      .WillOnce(Invoke(SaveFilePathTo(&file_path_2)));

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

  const StringType user_defined_portion = FILE_PATH_LITERAL("user_defined");
  const base::FilePath local_logs_base_path =
      local_logs_base_dir_.Append(user_defined_portion);

  ASSERT_TRUE(EnableLocalLogging(local_logs_base_path));
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path_1);
  ASSERT_FALSE(file_path_1->empty());

  // [user_defined]_[date]_[time]_[pid]_[lid].log
  const StringType date = FILE_PATH_LITERAL("20170906");
  const StringType time = FILE_PATH_LITERAL("1043");
  base::FilePath expected_path_1 = local_logs_base_path;
  expected_path_1 = local_logs_base_path.InsertBeforeExtension(
      FILE_PATH_LITERAL("_") + date + FILE_PATH_LITERAL("_") + time +
      FILE_PATH_LITERAL("_") + IntToStringType(rph_->GetID()) +
      FILE_PATH_LITERAL("_") + IntToStringType(kPeerConnectionId));
  expected_path_1 = expected_path_1.AddExtension(FILE_PATH_LITERAL("log"));

  ASSERT_EQ(file_path_1, expected_path_1);

  ASSERT_TRUE(DisableLocalLogging());
  ASSERT_TRUE(EnableLocalLogging(local_logs_base_path));
  ASSERT_TRUE(file_path_2);
  ASSERT_FALSE(file_path_2->empty());

  const base::FilePath expected_path_2 =
      expected_path_1.InsertBeforeExtension(FILE_PATH_LITERAL(" (1)"));

  // Focus of the test - starting the same log again produces a new file,
  // with an expected new filename.
  ASSERT_EQ(file_path_2, expected_path_2);
}

TEST_F(WebRtcEventLogManagerTest,
       OnRemoteLogStartedNotCalledIfRemoteLoggingNotEnabled) {
  MockWebRtcRemoteEventLogsObserver observer;
  EXPECT_CALL(observer, OnRemoteLogStarted(_, _)).Times(0);
  SetRemoteLogsObserver(&observer);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       OnRemoteLogStoppedNotCalledIfRemoteLoggingNotEnabled) {
  MockWebRtcRemoteEventLogsObserver observer;
  EXPECT_CALL(observer, OnRemoteLogStopped(_)).Times(0);
  SetRemoteLogsObserver(&observer);
  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));
}

TEST_F(WebRtcEventLogManagerTest,
       OnRemoteLogStartedCalledIfRemoteLoggingEnabled) {
  MockWebRtcRemoteEventLogsObserver observer;
  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);
  EXPECT_CALL(observer, OnRemoteLogStarted(key, _)).Times(1);
  SetRemoteLogsObserver(&observer);
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(StartRemoteLogging(key.render_process_id, key.lid));
}

TEST_F(WebRtcEventLogManagerTest,
       OnRemoteLogStoppedCalledIfRemoteLoggingEnabledThenPcRemoved) {
  MockWebRtcRemoteEventLogsObserver observer;
  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);
  EXPECT_CALL(observer, OnRemoteLogStopped(key)).Times(1);
  SetRemoteLogsObserver(&observer);
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(StartRemoteLogging(key.render_process_id, key.lid));
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));
}

TEST_F(WebRtcEventLogManagerTest,
       BrowserContextInitializationCreatesDirectoryForRemoteLogs) {
  auto browser_context = CreateBrowserContext();
  base::FilePath remote_logs_path = GetLogsDirectoryPath(browser_context.get());
  EXPECT_TRUE(base::DirectoryExists(remote_logs_path));
  EXPECT_TRUE(base::IsDirectoryEmpty(remote_logs_path));
}

// TODO: !!! Test that it finds old folders.

TEST_F(WebRtcEventLogManagerTest,
       StartRemoteLoggingReturnsFalseIfUnknownPeerConnection) {
  EXPECT_FALSE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       StartRemoteLoggingReturnsTrueIfKnownPeerConnection) {
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  EXPECT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       StartRemoteLoggingReturnsFalseIfRestartAttempt) {
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  EXPECT_FALSE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest,
       StartRemoteLoggingReturnsFalseIfUnlimitedFileSize) {
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  EXPECT_FALSE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId,
                                  kWebRtcEventLogManagerUnlimitedFileSize));
}

TEST_F(WebRtcEventLogManagerTest,
       StartRemoteLoggingReturnsFalseIfPeerConnectionAlreadyClosed) {
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
  EXPECT_FALSE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
}

TEST_F(WebRtcEventLogManagerTest, StartRemoteLoggingCreatesEmptyFile) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  base::Optional<base::FilePath> file_path;
  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);
  EXPECT_CALL(observer, OnRemoteLogStarted(key, _))
      .Times(1)
      .WillOnce(Invoke(SaveFilePathTo(&file_path)));
  SetRemoteLogsObserver(&observer);

  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));

  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, "");
}

TEST_F(WebRtcEventLogManagerTest, RemoteLogFileCreatedInCorrectDirectory) {
  // Set up separate browser contexts; each one will get one log.
  constexpr size_t kLogsNum = 3;
  std::unique_ptr<TestBrowserContext> browser_contexts[kLogsNum] = {
      CreateBrowserContext(), CreateBrowserContext(), CreateBrowserContext()};
  std::vector<std::unique_ptr<MockRenderProcessHost>> rphs;
  for (auto& browser_context : browser_contexts) {
    rphs.emplace_back(
        std::make_unique<MockRenderProcessHost>(browser_context.get()));
  }

  // Prepare to store the logs' paths in distinct memory locations.
  base::Optional<base::FilePath> file_paths[kLogsNum];
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  for (size_t i = 0; i < kLogsNum; i++) {
    const PeerConnectionKey key(rphs[i]->GetID(), kPeerConnectionId);
    EXPECT_CALL(observer, OnRemoteLogStarted(key, _))
        .Times(1)
        .WillOnce(Invoke(SaveFilePathTo(&file_paths[i])));
  }
  SetRemoteLogsObserver(&observer);

  // Start one log for each browser context.
  for (const auto& rph : rphs) {
    ASSERT_TRUE(PeerConnectionAdded(rph->GetID(), kPeerConnectionId));
    ASSERT_TRUE(StartRemoteLogging(rph->GetID(), kPeerConnectionId));
  }

  // All log files must be created in their own context's directory.
  for (size_t i = 0; i < arraysize(browser_contexts); i++) {
    ASSERT_TRUE(file_paths[i]);
    EXPECT_TRUE(browser_contexts[i]->GetPath().IsParent(*file_paths[i]));
  }
}

TEST_F(WebRtcEventLogManagerTest,
       OnWebRtcEventLogWriteWritesToTheRemoteBoundFile) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  base::Optional<base::FilePath> file_path;
  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);
  EXPECT_CALL(observer, OnRemoteLogStarted(key, _))
      .Times(1)
      .WillOnce(Invoke(SaveFilePathTo(&file_path)));
  SetRemoteLogsObserver(&observer);

  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));

  const char* const output = "1 + 1 = 3";
  EXPECT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, output),
            std::make_pair(false, true));

  std::string file_contents;
  EXPECT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, output);
}

TEST_F(WebRtcEventLogManagerTest, WriteToBothLocalAndRemoteFiles) {
  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));

  NiceMock<MockWebRtcLocalEventLogsObserver> local_observer;
  base::Optional<base::FilePath> local_path;
  EXPECT_CALL(local_observer, OnLocalLogStarted(key, _))
      .Times(1)
      .WillOnce(Invoke(SaveFilePathTo(&local_path)));
  SetLocalLogsObserver(&local_observer);

  NiceMock<MockWebRtcRemoteEventLogsObserver> remote_observer;
  base::Optional<base::FilePath> remote_path;
  EXPECT_CALL(remote_observer, OnRemoteLogStarted(key, _))
      .Times(1)
      .WillOnce(Invoke(SaveFilePathTo(&remote_path)));
  SetRemoteLogsObserver(&remote_observer);

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(StartRemoteLogging(key.render_process_id, key.lid));

  ASSERT_TRUE(local_path);
  ASSERT_FALSE(local_path->empty());
  ASSERT_TRUE(remote_path);
  ASSERT_FALSE(remote_path->empty());

  const char* const log = "logloglog";
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log),
            std::make_pair(true, true));

  // Ensure the flushing of the file to disk before attempting to read them.
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));

  for (base::Optional<base::FilePath> file_path : {local_path, remote_path}) {
    std::string file_contents;
    ASSERT_TRUE(file_path);
    ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
    EXPECT_EQ(file_contents, log);
  }
}

TEST_F(WebRtcEventLogManagerTest, MultipleWritesToSameRemoteBoundLogfile) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnRemoteLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(StartRemoteLogging(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  const std::string logs[] = {"ABC", "DEF", "XYZ"};
  for (const std::string& log : logs) {
    ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log),
              std::make_pair(false, true));
  }

  // Make sure the file would be closed, so that we could safely read it.
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents,
            std::accumulate(std::begin(logs), std::end(logs), std::string()));
}

TEST_F(WebRtcEventLogManagerTest, RemoteLogFileSizeLimitNotExceeded) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);
  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnRemoteLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

  const std::string log = "tpyo";
  const size_t file_size_limit_bytes = log.length() / 2;

  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(StartRemoteLogging(key.render_process_id, key.lid,
                                 file_size_limit_bytes));

  // A failure is reported, because not everything could be written. The file
  // will also be closed.
  EXPECT_CALL(observer, OnRemoteLogStopped(key)).Times(1);
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log),
            std::make_pair(false, false));

  // Additional calls to Write() have no effect.
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, "ignored"),
            std::make_pair(false, false));

  std::string file_contents;
  ASSERT_TRUE(file_path);
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents, log.substr(0, file_size_limit_bytes));
}

TEST_F(WebRtcEventLogManagerTest,
       LogMultipleActiveRemoteLogsSameBrowserContext) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  const std::vector<PeerConnectionKey> keys = {
      PeerConnectionKey(rph_->GetID(), 0), PeerConnectionKey(rph_->GetID(), 1),
      PeerConnectionKey(rph_->GetID(), 2)};

  std::vector<base::Optional<base::FilePath>> file_paths(keys.size());
  for (size_t i = 0; i < keys.size(); i++) {
    ON_CALL(observer, OnRemoteLogStarted(keys[i], _))
        .WillByDefault(Invoke(SaveFilePathTo(&file_paths[i])));
    ASSERT_TRUE(PeerConnectionAdded(keys[i].render_process_id, keys[i].lid));
    ASSERT_TRUE(StartRemoteLogging(keys[i].render_process_id, keys[i].lid));
    ASSERT_TRUE(file_paths[i]);
    ASSERT_FALSE(file_paths[i]->empty());
  }

  std::vector<std::string> logs;
  for (size_t i = 0; i < keys.size(); i++) {
    logs.emplace_back(std::to_string(rph_->GetID()) + std::to_string(i));
    ASSERT_EQ(
        OnWebRtcEventLogWrite(keys[i].render_process_id, keys[i].lid, logs[i]),
        std::make_pair(false, true));
  }

  // Make sure the file woulds be closed, so that we could safely read them.
  for (auto& key : keys) {
    ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));
  }

  for (size_t i = 0; i < keys.size(); i++) {
    std::string file_contents;
    ASSERT_TRUE(base::ReadFileToString(*file_paths[i], &file_contents));
    EXPECT_EQ(file_contents, logs[i]);
  }
}

TEST_F(WebRtcEventLogManagerTest,
       LogMultipleActiveRemoteLogsDifferentBrowserContexts) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  constexpr size_t kLogsNum = 3;
  std::unique_ptr<TestBrowserContext> browser_contexts[kLogsNum] = {
      CreateBrowserContext(), CreateBrowserContext(), CreateBrowserContext()};
  std::vector<std::unique_ptr<MockRenderProcessHost>> rphs;
  for (auto& browser_context : browser_contexts) {
    rphs.emplace_back(
        std::make_unique<MockRenderProcessHost>(browser_context.get()));
  }

  std::vector<PeerConnectionKey> keys;
  for (auto& rph : rphs) {
    keys.emplace_back(rph->GetID(), kPeerConnectionId);
  }

  std::vector<base::Optional<base::FilePath>> file_paths(keys.size());
  for (size_t i = 0; i < keys.size(); i++) {
    ON_CALL(observer, OnRemoteLogStarted(keys[i], _))
        .WillByDefault(Invoke(SaveFilePathTo(&file_paths[i])));
    ASSERT_TRUE(PeerConnectionAdded(keys[i].render_process_id, keys[i].lid));
    ASSERT_TRUE(StartRemoteLogging(keys[i].render_process_id, keys[i].lid));
    ASSERT_TRUE(file_paths[i]);
    ASSERT_FALSE(file_paths[i]->empty());
  }

  std::vector<std::string> logs;
  for (size_t i = 0; i < keys.size(); i++) {
    logs.emplace_back(std::to_string(rph_->GetID()) + std::to_string(i));
    ASSERT_EQ(
        OnWebRtcEventLogWrite(keys[i].render_process_id, keys[i].lid, logs[i]),
        std::make_pair(false, true));
  }

  // Make sure the file woulds be closed, so that we could safely read them.
  for (auto& key : keys) {
    ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));
  }

  for (size_t i = 0; i < keys.size(); i++) {
    std::string file_contents;
    ASSERT_TRUE(base::ReadFileToString(*file_paths[i], &file_contents));
    EXPECT_EQ(file_contents, logs[i]);
  }
}

TEST_F(WebRtcEventLogManagerTest, DifferentRemoteLogsMayHaveDifferentMaximums) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  const std::vector<PeerConnectionKey> keys = {
      PeerConnectionKey(rph_->GetID(), 0), PeerConnectionKey(rph_->GetID(), 1)};
  base::Optional<base::FilePath> file_paths[keys.size()];
  for (size_t i = 0; i < keys.size(); i++) {
    ON_CALL(observer, OnRemoteLogStarted(keys[i], _))
        .WillByDefault(Invoke(SaveFilePathTo(&file_paths[i])));
  }

  const size_t file_size_limits_bytes[2] = {3, 5};
  CHECK_EQ(arraysize(file_size_limits_bytes), keys.size());

  const std::string log = "lorem ipsum";

  for (size_t i = 0; i < keys.size(); i++) {
    ASSERT_TRUE(PeerConnectionAdded(keys[i].render_process_id, keys[i].lid));
    ASSERT_TRUE(StartRemoteLogging(keys[i].render_process_id, keys[i].lid,
                                   file_size_limits_bytes[i]));

    ASSERT_LT(file_size_limits_bytes[i], log.length());

    // A failure is reported, because not everything could be written. The file
    // will also be closed.
    ASSERT_EQ(
        OnWebRtcEventLogWrite(keys[i].render_process_id, keys[i].lid, log),
        std::make_pair(false, false));
  }

  for (size_t i = 0; i < keys.size(); i++) {
    std::string file_contents;
    ASSERT_TRUE(file_paths[i]);
    ASSERT_TRUE(base::ReadFileToString(*file_paths[i], &file_contents));
    EXPECT_EQ(file_contents, log.substr(0, file_size_limits_bytes[i]));
  }
}

TEST_F(WebRtcEventLogManagerTest, RemoteLogFileClosedWhenCapacityReached) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);
  base::Optional<base::FilePath> file_path;
  ON_CALL(observer, OnRemoteLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));

  const std::string log = "Let X equal X.";

  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(StartRemoteLogging(key.render_process_id, key.lid, log.length()));
  ASSERT_TRUE(file_path);

  EXPECT_CALL(observer, OnRemoteLogStopped(key)).Times(1);
  EXPECT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, log),
            std::make_pair(false, true));
}

#if defined(OS_POSIX) && !defined(OS_FUCHSIA)
// TODO(eladalon): Add unit tests for lacking read permissions when looking
// to upload the file. https://crbug.com/775415
TEST_F(WebRtcEventLogManagerTest,
       FailureToCreateRemoteLogsDirHandledGracefully) {
  base::ScopedTempDir browser_context_dir;
  ASSERT_TRUE(browser_context_dir.CreateUniqueTempDir());
  RemoveWritePermissionsFromDirectory(browser_context_dir.GetPath());

  // Graceful handling by BrowserContext::Initialize. (No crash, etc.)
  auto browser_context = CreateBrowserContext(browser_context_dir.Take());
  base::FilePath remote_logs_path = GetLogsDirectoryPath(browser_context.get());
  EXPECT_FALSE(base::DirectoryExists(remote_logs_path));

  auto rph = std::make_unique<MockRenderProcessHost>(browser_context.get());

  // Graceful handling of PeerConnectionAdded: True returned because the
  // remote-logs' manager can still safely reason about the state of peer
  // connections even if one of its browser contexts is defective.)
  EXPECT_TRUE(PeerConnectionAdded(rph->GetID(), kPeerConnectionId));

  // Graceful handling of StartRemoteLogging: False returned because it's
  // impossible to write the log to a file.
  EXPECT_FALSE(StartRemoteLogging(rph->GetID(), kPeerConnectionId));

  // Graceful handling of OnWebRtcEventLogWrite: False returned because the log
  // could not be written at all, let alone in its entirety.
  const char* const log = "This is not a log.";
  EXPECT_EQ(OnWebRtcEventLogWrite(rph->GetID(), kPeerConnectionId, log),
            std::make_pair(false, false));

  // Graceful handling of PeerConnectionRemoved: True returned because the
  // remote-logs' manager can still safely reason about the state of peer
  // connections even if one of its browser contexts is defective.
  EXPECT_TRUE(PeerConnectionRemoved(rph->GetID(), kPeerConnectionId));
}
#endif  // defined(OS_POSIX) && !defined(OS_FUCHSIA)

TEST_F(WebRtcEventLogManagerTest, GracefullyHandleFailureToStartRemoteLogFile) {
  StrictMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);  // WebRTC logging will not be turned on.

  // Set up a BrowserContext whose directory we know, so that we would be
  // able to manipulate it.
  base::ScopedTempDir browser_context_dir;
  ASSERT_TRUE(browser_context_dir.CreateUniqueTempDir());
  auto browser_context = CreateBrowserContext(browser_context_dir.Take());
  auto rph = std::make_unique<MockRenderProcessHost>(browser_context.get());

  // Remove write permissions from the directory.
  base::FilePath remote_logs_path = GetLogsDirectoryPath(browser_context.get());
  ASSERT_TRUE(base::DirectoryExists(remote_logs_path));
  RemoveWritePermissionsFromDirectory(remote_logs_path);

  // StartRemoteLogging() will now fail.
  const PeerConnectionKey key(rph->GetID(), kPeerConnectionId);
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  EXPECT_FALSE(StartRemoteLogging(key.render_process_id, key.lid));
  EXPECT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, "abc"),
            std::make_pair(false, false));
  EXPECT_TRUE(base::IsDirectoryEmpty(remote_logs_path));
}

TEST_F(WebRtcEventLogManagerTest, RemoteLogLimitActiveLogFiles) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  const int kMaxRemoteLogFiles =
      static_cast<int>(kMaxNumberActiveRemoteWebRtcEventLogFiles);

  for (int i = 0; i < kMaxRemoteLogFiles + 1; i++) {
    ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
  }

  int i;
  for (i = 0; i < kMaxRemoteLogFiles; i++) {
    EXPECT_CALL(observer,
                OnRemoteLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
        .Times(1);
    ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), i));
  }

  EXPECT_CALL(observer, OnRemoteLogStarted(_, _)).Times(0);
  EXPECT_FALSE(StartRemoteLogging(rph_->GetID(), i));
}

TEST_F(WebRtcEventLogManagerTest,
       RemoteLogFilledLogNotCountedTowardsLogsLimit) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  const std::string log = "very_short_log";

  const int kMaxRemoteLogFiles =
      static_cast<int>(kMaxNumberActiveRemoteWebRtcEventLogFiles);
  int i;
  for (i = 0; i < kMaxRemoteLogFiles; i++) {
    ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
    EXPECT_CALL(observer,
                OnRemoteLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
        .Times(1);
    ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), i, log.length()));
  }

  // By writing to one of the logs until it reaches capacity, we fill it,
  // causing it to close, therefore allowing an additional log.
  EXPECT_EQ(OnWebRtcEventLogWrite(rph_->GetID(), 0, log),
            std::make_pair(false, true));

  // We now have room for one additional log.
  ++i;
  EXPECT_CALL(observer,
              OnRemoteLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
      .Times(1);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), i));
}

TEST_F(WebRtcEventLogManagerTest,
       RemoteLogForRemovedPeerConnectionNotCountedTowardsLogsLimit) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  const int kMaxRemoteLogFiles =
      static_cast<int>(kMaxNumberActiveRemoteWebRtcEventLogFiles);
  int i;
  for (i = 0; i < kMaxRemoteLogFiles; i++) {
    ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
    EXPECT_CALL(observer,
                OnRemoteLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
        .Times(1);
    ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), i));
  }

  // By removing a peer connection associated with one of the logs, we allow
  // an additional log.
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), 0));

  // We now have room for one additional log.
  ++i;
  EXPECT_CALL(observer,
              OnRemoteLogStarted(PeerConnectionKey(rph_->GetID(), i), _))
      .Times(1);
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), i));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), i));
}

// TODO: !!! Cannot start new active logs if it would take us over the pending
// remote logs limit.

TEST_F(WebRtcEventLogManagerTest, RemoteLogSanityDestructorWithActiveFile) {
  // We don't actually test that the file was closed, because this behavior
  // is not supported - WebRtcEventLogManager's destructor may never be called,
  // except for in unit tests.
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
  DestroyUnitUnderTest();  // Explicitly show destruction does not crash.
}

TEST_F(WebRtcEventLogManagerTest, RemoteLogEmptyStringHandledGracefully) {
  NiceMock<MockWebRtcRemoteEventLogsObserver> observer;
  SetRemoteLogsObserver(&observer);

  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  // By writing a log after the empty string, we show that no odd behavior is
  // encountered, such as closing the file (an actual bug from WebRTC).
  const std::vector<std::string> logs = {"<setup>", "", "<encore>"};

  base::Optional<base::FilePath> file_path;

  ON_CALL(observer, OnRemoteLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&file_path)));
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(StartRemoteLogging(key.render_process_id, key.lid));
  ASSERT_TRUE(file_path);
  ASSERT_FALSE(file_path->empty());

  for (size_t i = 0; i < logs.size(); i++) {
    ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, logs[i]),
              std::make_pair(false, true));
  }
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));

  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(*file_path, &file_contents));
  EXPECT_EQ(file_contents,
            std::accumulate(std::begin(logs), std::end(logs), std::string()));
}

TEST_F(WebRtcEventLogManagerTest,
       UnponedRemoteLogFilesNotCountedTowardsActiveLogsLimit) {
  // Set up BrowserContexts whose directories we know, so that we would be
  // able to manipulate them.
  std::unique_ptr<TestBrowserContext> browser_contexts[2];
  std::unique_ptr<MockRenderProcessHost> rphs[2];
  for (size_t i = 0; i < 2; i++) {
    base::ScopedTempDir browser_context_dir;
    ASSERT_TRUE(browser_context_dir.CreateUniqueTempDir());
    browser_contexts[i] = CreateBrowserContext(browser_context_dir.Take());
    rphs[i] =
        std::make_unique<MockRenderProcessHost>(browser_contexts[i].get());
  }

  constexpr size_t without_permissions = 0;
  constexpr size_t with_permissions = 1;

  // Remove write permissions from one directory.
  base::FilePath permissions_lacking_remote_logs_path =
      GetLogsDirectoryPath(browser_contexts[without_permissions].get());
  ASSERT_TRUE(base::DirectoryExists(permissions_lacking_remote_logs_path));
  RemoveWritePermissionsFromDirectory(permissions_lacking_remote_logs_path);

  // Fail to start a log associated with the permission-lacking directory.
  ASSERT_TRUE(PeerConnectionAdded(rphs[without_permissions]->GetID(), 0));
  ASSERT_FALSE(StartRemoteLogging(rphs[without_permissions]->GetID(), 0));

  // Show that this was not counted towards the limit of active files.
  const int kMaxRemoteLogFiles =
      static_cast<int>(kMaxNumberActiveRemoteWebRtcEventLogFiles);
  for (int i = 0; i < kMaxRemoteLogFiles; i++) {
    ASSERT_TRUE(PeerConnectionAdded(rphs[with_permissions]->GetID(), i));
    EXPECT_TRUE(StartRemoteLogging(rphs[with_permissions]->GetID(), i));
  }
}

TEST_F(WebRtcEventLogManagerTest,
       NoStartWebRtcSendingEventLogsWhenLocalEnabledWithoutPeerConnection) {
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(EnableLocalLogging());
  EXPECT_TRUE(webrtc_state_change_instructions_.empty());
}

TEST_F(WebRtcEventLogManagerTest,
       NoStartWebRtcSendingEventLogsWhenPeerConnectionButNoLoggingEnabled) {
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  EXPECT_TRUE(webrtc_state_change_instructions_.empty());
}

TEST_F(WebRtcEventLogManagerTest,
       StartWebRtcSendingEventLogsWhenLocalEnabledThenPeerConnectionAdded) {
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);
}

TEST_F(WebRtcEventLogManagerTest,
       StartWebRtcSendingEventLogsWhenPeerConnectionAddedThenLocalEnabled) {
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);
}

TEST_F(WebRtcEventLogManagerTest,
       StartWebRtcSendingEventLogsWhenRemoteLoggingEnabled) {
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);
}

TEST_F(WebRtcEventLogManagerTest,
       InstructWebRtcToStopSendingEventLogsWhenLocalLoggingStopped) {
  // Setup
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);

  // Test
  ASSERT_TRUE(DisableLocalLogging());
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, false);
}

// #1 - Local logging was the cause of the logs.
TEST_F(WebRtcEventLogManagerTest,
       InstructWebRtcToStopSendingEventLogsWhenPeerConnectionRemoved1) {
  // Setup
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);

  // Test
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, false);
}

// #2 - Remote logging was the cause of the logs.
TEST_F(WebRtcEventLogManagerTest,
       InstructWebRtcToStopSendingEventLogsWhenPeerConnectionRemoved2) {
  // Setup
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);

  // Test
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, false);
}

// #1 - Local logging added first.
TEST_F(WebRtcEventLogManagerTest,
       SecondLoggingTargetDoesNotInitiateWebRtcLogging1) {
  // Setup
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);

  // Test
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  EXPECT_TRUE(webrtc_state_change_instructions_.empty());
}

// #2 - Remote logging added first.
TEST_F(WebRtcEventLogManagerTest,
       SecondLoggingTargetDoesNotInitiateWebRtcLogging2) {
  // Setup
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);

  // Test
  ASSERT_TRUE(EnableLocalLogging());
  EXPECT_TRUE(webrtc_state_change_instructions_.empty());
}

TEST_F(WebRtcEventLogManagerTest,
       DisablingLocalLoggingWhenRemoteLoggingEnabledDoesNotStopWebRtcLogging) {
  // Setup
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);

  // Test
  ASSERT_TRUE(DisableLocalLogging());
  EXPECT_TRUE(webrtc_state_change_instructions_.empty());

  // Cleanup
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, false);
}

TEST_F(WebRtcEventLogManagerTest,
       DisablingLocalLoggingAfterPcRemovalHasNoEffectOnWebRtcLogging) {
  // Setup
  SetPeerConnectionTrackerProxyForTesting(
      std::make_unique<PeerConnectionTrackerProxyForTesting>(this));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, true);

  // Test
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
  ExpectWebRtcStateChangeInstruction(rph_->GetID(), kPeerConnectionId, false);
  ASSERT_TRUE(DisableLocalLogging());
  EXPECT_TRUE(webrtc_state_change_instructions_.empty());
}

// Once a peer connection with a given key was removed, it may not again be
// added. But, if this impossible case occurs, WebRtcEventLogManager will
// not crash.
TEST_F(WebRtcEventLogManagerTest, SanityOverRecreatingTheSamePeerConnection) {
  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(StartRemoteLogging(rph_->GetID(), kPeerConnectionId));
  OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, "log1");
  ASSERT_TRUE(PeerConnectionRemoved(rph_->GetID(), kPeerConnectionId));
  ASSERT_TRUE(PeerConnectionAdded(rph_->GetID(), kPeerConnectionId));
  OnWebRtcEventLogWrite(rph_->GetID(), kPeerConnectionId, "log2");
}

// The logs would typically be binary. However, the other tests only cover ASCII
// characters, for readability. This test shows that this is not a problem.
TEST_F(WebRtcEventLogManagerTest, LogAllPossibleCharacters) {
  const PeerConnectionKey key(rph_->GetID(), kPeerConnectionId);

  NiceMock<MockWebRtcLocalEventLogsObserver> local_observer;
  SetLocalLogsObserver(&local_observer);
  base::Optional<base::FilePath> local_log_file_path;
  ON_CALL(local_observer, OnLocalLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&local_log_file_path)));

  NiceMock<MockWebRtcRemoteEventLogsObserver> remote_observer;
  SetRemoteLogsObserver(&remote_observer);
  base::Optional<base::FilePath> remote_log_file_path;
  ON_CALL(remote_observer, OnRemoteLogStarted(key, _))
      .WillByDefault(Invoke(SaveFilePathTo(&remote_log_file_path)));

  ASSERT_TRUE(EnableLocalLogging());
  ASSERT_TRUE(PeerConnectionAdded(key.render_process_id, key.lid));
  ASSERT_TRUE(StartRemoteLogging(key.render_process_id, key.lid));
  ASSERT_TRUE(local_log_file_path);
  ASSERT_FALSE(local_log_file_path->empty());
  ASSERT_TRUE(remote_log_file_path);
  ASSERT_FALSE(remote_log_file_path->empty());

  std::string all_chars;
  for (size_t i = 0; i < 256; i++) {
    all_chars += static_cast<uint8_t>(i);
  }
  ASSERT_EQ(OnWebRtcEventLogWrite(key.render_process_id, key.lid, all_chars),
            std::make_pair(true, true));

  // Make sure the file would be closed, so that we could safely read it.
  ASSERT_TRUE(PeerConnectionRemoved(key.render_process_id, key.lid));

  std::string local_log_file_contents;
  ASSERT_TRUE(
      base::ReadFileToString(*local_log_file_path, &local_log_file_contents));
  EXPECT_EQ(local_log_file_contents, all_chars);

  std::string remote_log_file_contents;
  ASSERT_TRUE(
      base::ReadFileToString(*remote_log_file_path, &remote_log_file_contents));
  EXPECT_EQ(remote_log_file_contents, all_chars);
}

}  // namespace content
