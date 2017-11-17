// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <memory>
#include <utility>

#include "base/location.h"
#include "base/posix/eintr_wrapper.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "components/arc/arc_session_impl.h"
#include "components/arc/common/arc_bridge.mojom.h"
#include "components/signin/core/account_id/account_id.h"
#include "components/user_manager/fake_user_manager.h"
#include "components/user_manager/scoped_user_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {
namespace {

constexpr char kFakeGmail[] = "user@gmail.com";

class FakeArcBridgeHost : public mojom::ArcBridgeHost {
 public:
  // ArcBridgeHost overrides.
  void OnAccessibilityHelperInstanceReady(
      mojom::AccessibilityHelperInstancePtr accessibility_helper_ptr) override {
  }
  void OnAppInstanceReady(mojom::AppInstancePtr app_ptr) override {}
  void OnAudioInstanceReady(mojom::AudioInstancePtr audio_ptr) override {}
  void OnAuthInstanceReady(mojom::AuthInstancePtr auth_ptr) override {}
  void OnBackupSettingsInstanceReady(
      mojom::BackupSettingsInstancePtr instance_ptr) override {}
  void OnBluetoothInstanceReady(
      mojom::BluetoothInstancePtr bluetooth_ptr) override {}
  void OnBootPhaseMonitorInstanceReady(
      mojom::BootPhaseMonitorInstancePtr boot_phase_monitor_ptr) override {}
  void OnCastReceiverInstanceReady(
      mojom::CastReceiverInstancePtr cast_receiver_ptr) override {}
  void OnCertStoreInstanceReady(
      mojom::CertStoreInstancePtr instance_ptr) override {}
  void OnClipboardInstanceReady(
      mojom::ClipboardInstancePtr clipboard_ptr) override {}
  void OnCrashCollectorInstanceReady(
      mojom::CrashCollectorInstancePtr crash_collector_ptr) override {}
  void OnEnterpriseReportingInstanceReady(
      mojom::EnterpriseReportingInstancePtr enterprise_reporting_ptr) override {
  }
  void OnFileSystemInstanceReady(
      mojom::FileSystemInstancePtr file_system_ptr) override {}
  void OnImeInstanceReady(mojom::ImeInstancePtr ime_ptr) override {}
  void OnIntentHelperInstanceReady(
      mojom::IntentHelperInstancePtr intent_helper_ptr) override {}
  void OnKioskInstanceReady(mojom::KioskInstancePtr kiosk_ptr) override {}
  void OnLockScreenInstanceReady(
      mojom::LockScreenInstancePtr lock_screen_ptr) override {}
  void OnMetricsInstanceReady(mojom::MetricsInstancePtr metrics_ptr) override {}
  void OnMidisInstanceReady(mojom::MidisInstancePtr midis_ptr) override {}
  void OnNetInstanceReady(mojom::NetInstancePtr net_ptr) override {}
  void OnNotificationsInstanceReady(
      mojom::NotificationsInstancePtr notifications_ptr) override {}
  void OnObbMounterInstanceReady(
      mojom::ObbMounterInstancePtr obb_mounter_ptr) override {}
  void OnOemCryptoInstanceReady(
      mojom::OemCryptoInstancePtr oemcrypto_ptr) override {}
  void OnPolicyInstanceReady(mojom::PolicyInstancePtr policy_ptr) override {}
  void OnPowerInstanceReady(mojom::PowerInstancePtr power_ptr) override {}
  void OnPrintInstanceReady(mojom::PrintInstancePtr print_ptr) override {}
  void OnProcessInstanceReady(mojom::ProcessInstancePtr process_ptr) override {}
  void OnRotationLockInstanceReady(
      mojom::RotationLockInstancePtr rotation_lock_ptr) override {}
  void OnStorageManagerInstanceReady(
      mojom::StorageManagerInstancePtr storage_manager_ptr) override {}
  void OnTracingInstanceReady(mojom::TracingInstancePtr trace_ptr) override {}
  void OnTtsInstanceReady(mojom::TtsInstancePtr tts_ptr) override {}
  void OnVideoInstanceReady(mojom::VideoInstancePtr video_ptr) override {}
  void OnVoiceInteractionArcHomeInstanceReady(
      mojom::VoiceInteractionArcHomeInstancePtr home_ptr) override {}
  void OnVoiceInteractionFrameworkInstanceReady(
      mojom::VoiceInteractionFrameworkInstancePtr framework_ptr) override {}
  void OnVolumeMounterInstanceReady(
      mojom::VolumeMounterInstancePtr volume_mounter_ptr) override {}
  void OnWallpaperInstanceReady(
      mojom::WallpaperInstancePtr wallpaper_ptr) override {}
};

class FakeDelegate : public ArcSessionImpl::Delegate {
 public:
  // Controls whether the result of mojo connection emulation should be
  // success or fail.
  void set_success(bool success) { success_ = success; }

  // If set true, do not call the |callback| in ConnectMojo even
  // asynchronously.
  void set_suspend(bool suspend) { suspend_ = suspend; }

  // Runs the pending callback.
  void Resume() {
    DCHECK(!pending_callback_.is_null());
    RunCallback(std::move(pending_callback_));
  }

  // ArcSessionImpl::Delegate override:
  base::ScopedFD ConnectMojo(base::ScopedFD socket_fd,
                             ConnectMojoCallback callback) override {
    if (suspend_) {
      DCHECK(pending_callback_.is_null());
      pending_callback_ = std::move(callback);
    } else {
      RunCallback(std::move(callback));
    }

    // Open /dev/null as a dummy FD.
    return base::ScopedFD(HANDLE_EINTR(open("/dev/null", O_RDONLY)));
  }

 private:
  void RunCallback(ConnectMojoCallback callback) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            std::move(callback),
            success_ ? std::make_unique<FakeArcBridgeHost>() : nullptr));
  }

  bool success_ = true;
  bool suspend_ = false;
  ConnectMojoCallback pending_callback_;
};

class TestObserver : public ArcSession::Observer {
 public:
  struct Args {
    ArcStopReason reason;
    bool was_running;
  };

  explicit TestObserver(ArcSession* arc_session) : arc_session_(arc_session) {
    arc_session_->AddObserver(this);
  }

  ~TestObserver() override { arc_session_->RemoveObserver(this); }

  const base::Optional<Args>& args() const { return args_; }

  void OnSessionStopped(ArcStopReason reason, bool was_running) override {
    args_.emplace(Args{reason, was_running});
  }

 private:
  ArcSession* const arc_session_;  // Now owned.
  base::Optional<Args> args_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

// Custom deleter for ArcSession testing.
struct ArcSessionDeleter {
  void operator()(ArcSession* arc_session) {
    // ArcSessionImpl must be in STOPPED state, if the instance is being
    // destroyed. Calling OnShutdown() just before ensures it.
    arc_session->OnShutdown();
    delete arc_session;
  }
};

class ArcSessionImplTest : public testing::Test {
 public:
  ArcSessionImplTest() {
    chromeos::DBusThreadManager::GetSetterForTesting()->SetSessionManagerClient(
        std::make_unique<chromeos::FakeSessionManagerClient>());
    GetSessionManagerClient()->set_arc_available(true);

    // Create a user and set it as the primary user.
    const AccountId account_id = AccountId::FromUserEmail(kFakeGmail);
    const user_manager::User* user = GetUserManager()->AddUser(account_id);
    GetUserManager()->UserLoggedIn(account_id, user->username_hash(), false);
  }

  ~ArcSessionImplTest() override {
    GetUserManager()->RemoveUserFromList(AccountId::FromUserEmail(kFakeGmail));
    chromeos::DBusThreadManager::Shutdown();
  }

  chromeos::FakeSessionManagerClient* GetSessionManagerClient() {
    return static_cast<chromeos::FakeSessionManagerClient*>(
        chromeos::DBusThreadManager::Get()->GetSessionManagerClient());
  }

  user_manager::FakeUserManager* GetUserManager() {
    return static_cast<user_manager::FakeUserManager*>(
        user_manager::UserManager::Get());
  }

  std::unique_ptr<ArcSessionImpl, ArcSessionDeleter> CreateArcSession(
      std::unique_ptr<ArcSessionImpl::Delegate> delegate = nullptr) {
    if (!delegate)
      delegate = std::make_unique<FakeDelegate>();
    return std::unique_ptr<ArcSessionImpl, ArcSessionDeleter>(
        new ArcSessionImpl(std::move(delegate)));
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  user_manager::ScopedUserManager scoped_user_manager_{
      std::make_unique<user_manager::FakeUserManager>()};
};

TEST_F(ArcSessionImplTest, FullInstance_Success) {
  // Simple case. Start FULL_INSTANCE will eventually starts the container.
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::RUNNING_FULL_INSTANCE,
            arc_session->GetStateForTesting());
  EXPECT_FALSE(observer.args().has_value());
}

TEST_F(ArcSessionImplTest, FullInstance_DBusFail) {
  // Let SessionManagerClient::StartArcInstance() fail.
  GetSessionManagerClient()->set_arc_available(false);

  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::GENERIC_BOOT_FAILURE, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, FullInstance_LowDisk) {
  // Let SessionManagerClient::StartArcInstance() fail due to low disk.
  GetSessionManagerClient()->set_low_disk(true);

  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::LOW_DISK_SPACE, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, FullInstance_MojoConnectionFail) {
  // Let Mojo connection fail.
  auto delegate = std::make_unique<FakeDelegate>();
  delegate->set_success(false);

  auto arc_session = CreateArcSession(std::move(delegate));
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::GENERIC_BOOT_FAILURE, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, MiniInstance_Success) {
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::MINI_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::RUNNING_MINI_INSTANCE,
            arc_session->GetStateForTesting());
  EXPECT_FALSE(observer.args().has_value());
}

TEST_F(ArcSessionImplTest, MiniInstance_DBusFail) {
  // Let SessionManagerClient::StartArcInstance() fail.
  GetSessionManagerClient()->set_arc_available(false);

  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::MINI_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::GENERIC_BOOT_FAILURE, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, MiniInstance_LowDisk) {
  // Let SessionManagerClient::StartArcInstance() fail due to low disk.
  GetSessionManagerClient()->set_low_disk(true);

  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::MINI_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::LOW_DISK_SPACE, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, Upgrade_Success) {
  // Start a mini instance.
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::MINI_INSTANCE);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(ArcSessionImpl::State::RUNNING_MINI_INSTANCE,
            arc_session->GetStateForTesting());
  ASSERT_FALSE(observer.args().has_value());

  // Then, upgrade to a full instance.
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::RUNNING_FULL_INSTANCE,
            arc_session->GetStateForTesting());
  EXPECT_FALSE(observer.args().has_value());
}

TEST_F(ArcSessionImplTest, Upgrade_DBusFail) {
  // Start a mini instance.
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::MINI_INSTANCE);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(ArcSessionImpl::State::RUNNING_MINI_INSTANCE,
            arc_session->GetStateForTesting());

  // Hereafter, let SessionManagerClient::StartArcInstance() fail.
  GetSessionManagerClient()->set_arc_available(false);

  // Then upgrade, which should fail.
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::GENERIC_BOOT_FAILURE, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, Upgrade_MojoConnectionFail) {
  // Let Mojo connection fail.
  auto delegate = std::make_unique<FakeDelegate>();
  delegate->set_success(false);

  auto arc_session = CreateArcSession(std::move(delegate));
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::MINI_INSTANCE);
  base::RunLoop().RunUntilIdle();
  // Starting mini instance should succeed, because it is not related to
  // Mojo connection.
  ASSERT_EQ(ArcSessionImpl::State::RUNNING_MINI_INSTANCE,
            arc_session->GetStateForTesting());

  // Upgrade should fail, due to Mojo connection fail set above.
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::GENERIC_BOOT_FAILURE, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, Upgrade_StartingMiniInstance) {
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::MINI_INSTANCE);
  ASSERT_EQ(ArcSessionImpl::State::STARTING_MINI_INSTANCE,
            arc_session->GetStateForTesting());

  // Before moving forward to RUNNING_MINI_INSTANCE, start upgrading it.
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);

  // The state should not immediately switch to STARTING_FULL_INSTANCE, yet.
  EXPECT_EQ(ArcSessionImpl::State::STARTING_MINI_INSTANCE,
            arc_session->GetStateForTesting());

  // Complete the upgrade procedure.
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::RUNNING_FULL_INSTANCE,
            arc_session->GetStateForTesting());
  EXPECT_FALSE(observer.args().has_value());
}

TEST_F(ArcSessionImplTest, Stop_StartingMiniInstance) {
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::MINI_INSTANCE);
  ASSERT_EQ(ArcSessionImpl::State::STARTING_MINI_INSTANCE,
            arc_session->GetStateForTesting());

  arc_session->Stop();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::SHUTDOWN, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, Stop_RunningMiniInstance) {
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::MINI_INSTANCE);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(ArcSessionImpl::State::RUNNING_MINI_INSTANCE,
            arc_session->GetStateForTesting());

  arc_session->Stop();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::SHUTDOWN, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, Stop_StartingFullInstance) {
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  ASSERT_EQ(ArcSessionImpl::State::STARTING_FULL_INSTANCE,
            arc_session->GetStateForTesting());

  arc_session->Stop();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::SHUTDOWN, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, Stop_ConnectingMojo) {
  // Let Mojo connection suspend.
  auto delegate = std::make_unique<FakeDelegate>();
  delegate->set_suspend(true);
  auto resume_callback =
      base::BindOnce(&FakeDelegate::Resume, base::Unretained(delegate.get()));
  auto arc_session = CreateArcSession(std::move(delegate));
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(ArcSessionImpl::State::CONNECTING_MOJO,
            arc_session->GetStateForTesting());

  arc_session->Stop();
  std::move(resume_callback).Run();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::SHUTDOWN, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, Stop_Running) {
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(ArcSessionImpl::State::RUNNING_FULL_INSTANCE,
            arc_session->GetStateForTesting());

  arc_session->Stop();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::SHUTDOWN, observer.args()->reason);
  EXPECT_TRUE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, Stop_ConflictWithFailure) {
  // Let SessionManagerClient::StartArcInstance() fail.
  GetSessionManagerClient()->set_arc_available(false);

  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  ASSERT_EQ(ArcSessionImpl::State::STARTING_FULL_INSTANCE,
            arc_session->GetStateForTesting());

  arc_session->Stop();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  // Even if D-Bus reports an error, if Stop() is invoked, it will be handled
  // as clean shutdown.
  EXPECT_EQ(ArcStopReason::SHUTDOWN, observer.args()->reason);
  EXPECT_FALSE(observer.args()->was_running);
}

// Emulating crash.
TEST_F(ArcSessionImplTest, ArcStopInstance) {
  auto arc_session = CreateArcSession();
  TestObserver observer(arc_session.get());
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(ArcSessionImpl::State::RUNNING_FULL_INSTANCE,
            arc_session->GetStateForTesting());

  // Deliver the ArcInstanceStopped D-Bus signal.
  auto* session_manager_client = GetSessionManagerClient();
  session_manager_client->NotifyArcInstanceStopped(
      false /* meaning crash */,
      session_manager_client->container_instance_id());

  EXPECT_EQ(ArcSessionImpl::State::STOPPED, arc_session->GetStateForTesting());
  ASSERT_TRUE(observer.args().has_value());
  EXPECT_EQ(ArcStopReason::CRASH, observer.args()->reason);
  EXPECT_TRUE(observer.args()->was_running);
}

TEST_F(ArcSessionImplTest, ArcStopInstance_WrongContainerInstanceId) {
  auto arc_session = CreateArcSession();
  arc_session->Start(ArcInstanceMode::FULL_INSTANCE);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(ArcSessionImpl::State::RUNNING_FULL_INSTANCE,
            arc_session->GetStateForTesting());

  // Deliver the ArcInstanceStopped D-Bus signal.
  auto* session_manager_client = GetSessionManagerClient();
  session_manager_client->NotifyArcInstanceStopped(false /* meaning crash */,
                                                   "dummy instance id");

  // The signal should be ignored.
  EXPECT_EQ(ArcSessionImpl::State::RUNNING_FULL_INSTANCE,
            arc_session->GetStateForTesting());
}

}  // namespace
}  // namespace arc
