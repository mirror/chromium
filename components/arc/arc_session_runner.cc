// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/arc_session_runner.h"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/posix/eintr_wrapper.h"
#include "base/sys_info.h"
#include "base/task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_checker.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/arc/arc_bridge_host_impl.h"
#include "components/arc/arc_features.h"
#include "components/arc/arc_util.h"
#include "components/user_manager/user_manager.h"

namespace arc {

namespace {

constexpr base::TimeDelta kDefaultRestartDelay =
    base::TimeDelta::FromSeconds(5);

// This is called when StopArcInstance D-Bus method completes. Since we have the
// ArcInstanceStopped() callback and are notified if StartArcInstance fails, we
// don't need to do anything when StopArcInstance completes.
void DoNothingInstanceStopped(bool) {}

chromeos::SessionManagerClient* GetSessionManagerClient() {
  // If the DBusThreadManager or the SessionManagerClient aren't available,
  // there isn't much we can do. This should only happen when running tests.
  if (!chromeos::DBusThreadManager::IsInitialized() ||
      !chromeos::DBusThreadManager::Get() ||
      !chromeos::DBusThreadManager::Get()->GetSessionManagerClient())
    return nullptr;
  return chromeos::DBusThreadManager::Get()->GetSessionManagerClient();
}

// Creates a pipe. Returns true on success, otherwise false.
// On success, |read_fd| will be set to the fd of the read side, and
// |write_fd| will be set to the one of write side.
bool CreatePipe(base::ScopedFD* read_fd, base::ScopedFD* write_fd) {
  int fds[2];
  if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) < 0) {
    PLOG(ERROR) << "pipe2()";
    return false;
  }

  read_fd->reset(fds[0]);
  write_fd->reset(fds[1]);
  return true;
}

// Waits until |raw_socket_fd| is readable.
// The operation may be cancelled originally triggered by user interaction to
// disable ARC, or ARC instance is unexpectedly stopped (e.g. crash).
// To notify such a situation, |raw_cancel_fd| is also passed to here, and the
// write side will be closed in such a case.
bool WaitForSocketReadable(int raw_socket_fd, int raw_cancel_fd) {
  struct pollfd fds[2] = {
      {raw_socket_fd, POLLIN, 0}, {raw_cancel_fd, POLLIN, 0},
  };

  if (HANDLE_EINTR(poll(fds, arraysize(fds), -1)) <= 0) {
    PLOG(ERROR) << "poll()";
    return false;
  }

  if (fds[1].revents) {
    // Notified that Stop() is invoked. Cancel the Mojo connecting.
    VLOG(1) << "Stop() was called during ConnectMojo()";
    return false;
  }

  DCHECK(fds[0].revents);
  return true;
}

}  // namespace

ArcSessionRunner::ArcSessionRunner(ArcBridgeService* arc_bridge_service)
    : restart_delay_(kDefaultRestartDelay),
      arc_bridge_service_(arc_bridge_service),
      weak_ptr_factory_(this) {
  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  if (client)
    client->AddObserver(this);
}

ArcSessionRunner::~ArcSessionRunner() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  if (client)
    client->RemoveObserver(this);
}

void ArcSessionRunner::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  observer_list_.AddObserver(observer);
}

void ArcSessionRunner::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  observer_list_.RemoveObserver(observer);
}

void ArcSessionRunner::RequestStart() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Consecutive RequestStart() call. Do nothing.
  if (target_state_ == TargetState::FULL)
    return;
  target_state_ = TargetState::FULL;
  VLOG(1) << "Session start requested";
  // Here |target_state_| transitions to FULL. So, |restart_timer_|
  // must be stopped (either not even started, or has been cancelled in
  // previous RequestStop() call).
  DCHECK(!restart_timer_.IsRunning());

  if (state_ == State::STOPPED)
    Start();
}

void ArcSessionRunner::RequestStop() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (target_state_ == TargetState::STOP)
    return;

  target_state_ = TargetState::STOP;
  VLOG(1) << "Session stop requested";
  arc_bridge_host_.reset();
  restart_timer_.Stop();
  accept_cancel_pipe_.reset();

  if (state_ == State::RUNNING)
    Stop();
}

void ArcSessionRunner::OnShutdown() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  VLOG(1) << "OnShutdown";

  target_state_ = TargetState::STOP;
  restart_timer_.Stop();
  if (state_ != State::STOPPED && state_ != State::STOPPING) {
    // Here, the message loop is already stopped, and the Chrome will be soon
    // shutdown. Thus, it is not necessary to take care about restarting case.
    // If ArcSession is waiting for mojo connection, cancels it.
    accept_cancel_pipe_.reset();
    Stop();

    // Directly set to the STOPPED state by OnStopped(). Note that calling
    // StopArcInstance() may not work well. At least, because the UI thread is
    // already stopped here, ArcInstanceStopped() callback cannot be invoked.
    OnStopped(ArcStopReason::SHUTDOWN);
  }
}

bool ArcSessionRunner::IsRunning() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return state_ == State::RUNNING;
}

bool ArcSessionRunner::IsStopped() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return state_ == State::STOPPED;
}

bool ArcSessionRunner::IsLoginScreenInstanceStarting() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return state_ == State::STARTING && target_state_ == TargetState::MINI;
}

void ArcSessionRunner::SetRestartDelayForTesting(
    const base::TimeDelta& restart_delay) {
  DCHECK_EQ(state_, State::STOPPED);
  DCHECK(!restart_timer_.IsRunning());
  restart_delay_ = restart_delay;
}

void ArcSessionRunner::Start() {
  state_ = State::STARTING;
  VLOG(1) << "Sending signal to start ARC instance.";
  SendStartArcInstanceDBusMessage(
      false /* instance_is_for_login_screen */,
      base::Bind(&ArcSessionRunner::OnInstanceStarted,
                 weak_ptr_factory_.GetWeakPtr(),
                 false /* instance_is_for_login_screen */));
}

void ArcSessionRunner::Stop() {
  state_ = State::STOPPING;
  VLOG(1) << "Sending signal to stop ARC instance.";
  chromeos::SessionManagerClient* session_manager_client =
      chromeos::DBusThreadManager::Get()->GetSessionManagerClient();
  session_manager_client->StopArcInstance(
      base::Bind(&DoNothingInstanceStopped));
}

void ArcSessionRunner::RestartArcSession() {
  if (target_state_ != TargetState::FULL)
    return;

  VLOG(0) << "Restarting ARC instance";
  // The order is important here. Call Start(), then notify observers.
  Start();
  for (auto& observer : observer_list_)
    observer.OnSessionRestarting();
}

void ArcSessionRunner::ArcInstanceStopped(
    bool clean,
    const std::string& container_instance_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(1) << "Notified that ARC instance is stopped "
          << (clean ? "cleanly" : "uncleanly");

  if (container_instance_id != container_instance_id_) {
    VLOG(1) << "Container instance id mismatch. Do nothing."
            << container_instance_id << " vs " << container_instance_id_;
    return;
  }

  // Release |container_instance_id_| to avoid duplicate invocation situation.
  container_instance_id_.clear();

  // In case that crash happens during before the Mojo channel is connected,
  // unlock the TaskScheduler's thread.
  accept_cancel_pipe_.reset();

  ArcStopReason reason;
  if (target_state_ == TargetState::STOP) {
    // If the ARC instance is stopped after its explicit request,
    // return SHUTDOWN.
    reason = ArcStopReason::SHUTDOWN;
  } else if (clean) {
    // If the ARC instance is stopped, but it is not explicitly requested,
    // then this is triggered by some failure during the starting procedure.
    // Return GENERIC_BOOT_FAILURE for the case.
    reason = ArcStopReason::GENERIC_BOOT_FAILURE;
  } else {
    // Otherwise, this is caused by CRASH occured inside of the ARC instance.
    reason = ArcStopReason::CRASH;
  }
  OnStopped(reason);
}

void ArcSessionRunner::OnStopped(ArcStopReason reason) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_NE(state_, State::STOPPED);
  VLOG(2) << "ARC session is stopped.";
  arc_bridge_host_.reset();

  // Restart ARC instance if it's supposed to be running and it crashed
  // unexpectedly.
  // If ARC instance crashed while starting or stopped (i.e. state_ !=
  // RUNNING), don't restart it.
  // TODO:cmtm what if state_ == CONNECTING_MOJO ?
  if (target_state_ == TargetState::FULL) {
    bool restarting = false;
    if (state_ == State::RUNNING || state_ == State::STOPPING) {
      // Note that even when |restart_delay_| is 0 (for testing), it needs to
      // PostTask, because observer callback may call RequestStart()/Stop().
      VLOG(0) << "Scheduling ARC to restart in " << restart_delay_;
      restarting = true;
      restart_timer_.Start(FROM_HERE, restart_delay_,
                           base::Bind(&ArcSessionRunner::RestartArcSession,
                                      weak_ptr_factory_.GetWeakPtr()));
    } else {
      target_state_ = TargetState::STOP;
    }

    for (auto& observer : observer_list_)
      observer.OnSessionStopped(reason, restarting);

  } else if (target_state_ == TargetState::MINI) {
    // The observers should be agnostic to the existence of the mini-container
    // instance.
    target_state_ = TargetState::STOP;
  }
  state_ = State::STOPPED;
}

// TODO:cmtm merge this function into Start()
// static
void ArcSessionRunner::SendStartArcInstanceDBusMessage(
    bool instance_is_for_login_screen,
    const chromeos::SessionManagerClient::StartArcInstanceCallback& cb) {
  chromeos::SessionManagerClient* session_manager_client =
      chromeos::DBusThreadManager::Get()->GetSessionManagerClient();
  const bool native_bridge_experiment =
      base::FeatureList::IsEnabled(arc::kNativeBridgeExperimentFeature);
  if (instance_is_for_login_screen) {
    session_manager_client->StartArcInstance(
        chromeos::SessionManagerClient::ArcStartupMode::LOGIN_SCREEN,
        // All variables below except |cb| will be ignored.
        cryptohome::Identification(), false, false, native_bridge_experiment,
        cb);
    return;
  }

  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  DCHECK(user_manager->GetPrimaryUser());
  const cryptohome::Identification cryptohome_id(
      user_manager->GetPrimaryUser()->GetAccountId());

  const bool skip_boot_completed_broadcast =
      !base::FeatureList::IsEnabled(arc::kBootCompletedBroadcastFeature);

  // We only enable /vendor/priv-app when voice interaction is enabled because
  // voice interaction service apk would be bundled in this location.
  const bool scan_vendor_priv_app =
      chromeos::switches::IsVoiceInteractionEnabled();

  session_manager_client->StartArcInstance(
      chromeos::SessionManagerClient::ArcStartupMode::FULL, cryptohome_id,
      skip_boot_completed_broadcast, scan_vendor_priv_app,
      native_bridge_experiment, cb);
}

void ArcSessionRunner::OnInstanceStarted(
    bool instance_is_for_login_screen,
    StartArcInstanceResult result,
    const std::string& container_instance_id,
    base::ScopedFD socket_fd) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, State::STARTING);

  if (result != StartArcInstanceResult::SUCCESS) {
    if (target_state_ == TargetState::STOP) {
      OnStopped(ArcStopReason::SHUTDOWN);
    } else {
      LOG(ERROR) << "Failed to start ARC instance";
      OnStopped(result == StartArcInstanceResult::LOW_FREE_DISK_SPACE
                    ? ArcStopReason::LOW_DISK_SPACE
                    : ArcStopReason::GENERIC_BOOT_FAILURE);
    }
  } else {
    switch (target_state_) {
      case TargetState::STOP:
        // The ARC instance has started to run. Request to stop.
        Stop();
        break;
      case TargetState::MINI:
        state_ = State::RUNNING;
        VLOG(2) << "ARC mini-container started successfully.";
        break;
      case TargetState::FULL:
        if (instance_is_for_login_screen) {
          VLOG(2) << "Upgrading an existing ARC mini-container to a full one.";
          Start();
        } else {
          VLOG(2) << "ARC instance is successfully started. Connecting Mojo...";
          state_ = State::CONNECTING_MOJO;

          // Prepare a pipe so that AcceptInstanceConnection can be interrupted
          // on Stop().
          base::ScopedFD cancel_fd;
          if (!CreatePipe(&cancel_fd, &accept_cancel_pipe_)) {
            Stop();
            return;
          }

          // TODO:cmtm look into this. what's relation of ConnectMojo?
          // For production, |socket_fd| passed from session_manager is either a
          // valid socket or a valid file descriptor (/dev/null). For testing,
          // |socket_fd| might be invalid.
          mojo::edk::PlatformHandle raw_handle(socket_fd.release());
          raw_handle.needs_connection = true;

          mojo::edk::ScopedPlatformHandle mojo_socket_fd(raw_handle);
          base::PostTaskWithTraitsAndReplyWithResult(
              FROM_HERE, {base::MayBlock()},
              base::Bind(&ArcSessionRunner::ConnectMojo,
                         base::Passed(&mojo_socket_fd),
                         base::Passed(&cancel_fd)),
              base::Bind(&ArcSessionRunner::OnMojoConnected,
                         weak_ptr_factory_.GetWeakPtr()));
        }
        break;
    }
  }
}

// static
mojo::ScopedMessagePipeHandle ArcSessionRunner::ConnectMojo(
    mojo::edk::ScopedPlatformHandle socket_fd,
    base::ScopedFD cancel_fd) {
  if (!WaitForSocketReadable(socket_fd.get().handle, cancel_fd.get())) {
    VLOG(1) << "Mojo connection was cancelled.";
    return mojo::ScopedMessagePipeHandle();
  }

  mojo::edk::ScopedPlatformHandle scoped_fd;
  if (!mojo::edk::ServerAcceptConnection(socket_fd.get(), &scoped_fd,
                                         /* check_peer_user = */ false) ||
      !scoped_fd.is_valid()) {
    return mojo::ScopedMessagePipeHandle();
  }

  // Hardcode pid 0 since it is unused in mojo.
  const base::ProcessHandle kUnusedChildProcessHandle = 0;
  mojo::edk::PlatformChannelPair channel_pair;
  mojo::edk::OutgoingBrokerClientInvitation invitation;

  std::string token = mojo::edk::GenerateRandomToken();
  mojo::ScopedMessagePipeHandle pipe = invitation.AttachMessagePipe(token);

  invitation.Send(
      kUnusedChildProcessHandle,
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  channel_pair.PassServerHandle()));

  mojo::edk::ScopedPlatformHandleVectorPtr handles(
      new mojo::edk::PlatformHandleVector{
          channel_pair.PassClientHandle().release()});

  // We need to send the length of the message as a single byte, so make sure it
  // fits.
  DCHECK_LT(token.size(), 256u);
  uint8_t message_length = static_cast<uint8_t>(token.size());
  struct iovec iov[] = {{&message_length, sizeof(message_length)},
                        {const_cast<char*>(token.c_str()), token.size()}};
  ssize_t result = mojo::edk::PlatformChannelSendmsgWithHandles(
      scoped_fd.get(), iov, sizeof(iov) / sizeof(iov[0]), handles->data(),
      handles->size());
  if (result == -1) {
    PLOG(ERROR) << "sendmsg";
    return mojo::ScopedMessagePipeHandle();
  }

  return pipe;
}

void ArcSessionRunner::OnMojoConnected(
    mojo::ScopedMessagePipeHandle server_pipe) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, State::CONNECTING_MOJO);
  accept_cancel_pipe_.reset();

  if (target_state_ == TargetState::STOP) {
    Stop();
    return;
  }

  if (!server_pipe.is_valid()) {
    LOG(ERROR) << "Invalid pipe";
    Stop();
    return;
  }

  mojom::ArcBridgeInstancePtr instance;
  instance.Bind(mojo::InterfacePtrInfo<mojom::ArcBridgeInstance>(
      std::move(server_pipe), 0u));
  arc_bridge_host_ = base::MakeUnique<ArcBridgeHostImpl>(arc_bridge_service_,
                                                         std::move(instance));

  VLOG(2) << "Mojo is connected. ARC is running.";
  state_ = State::RUNNING;
}

void ArcSessionRunner::EmitLoginPromptVisibleCalled() {
  if (ShouldArcOnlyStartAfterLogin()) {
    // Skip starting ARC for now. We'll have another chance to start the full
    // instance after the user logs in.
    return;
  }
  // Since 'login-prompt-visible' Upstart signal starts all Upstart jobs the
  // container may depend on such as cras, EmitLoginPromptVisibleCalled() is the
  // safe place to start the container for login screen.
  DCHECK_EQ(state_, State::STOPPED);
  state_ = State::STARTING;
  target_state_ = TargetState::MINI;

  SendStartArcInstanceDBusMessage(
      true /* instance_is_for_login_screen */,
      base::Bind(&ArcSessionRunner::OnInstanceStarted,
                 weak_ptr_factory_.GetWeakPtr(), true));
}

}  // namespace arc
