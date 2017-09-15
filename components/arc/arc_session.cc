// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/arc_session.h"

#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <unistd.h>

#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/posix/eintr_wrapper.h"
#include "base/sys_info.h"
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
#include "components/user_manager/user_manager.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"
#include "mojo/edk/embedder/platform_handle_vector.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace arc {

namespace {

using StartArcInstanceResult =
    chromeos::SessionManagerClient::StartArcInstanceResult;

// This is called when StopArcInstance D-Bus method completes. Since we have the
// ArcInstanceStopped() callback and are notified if StartArcInstance fails, we
// don't need to do anything when StopArcInstance completes.
void DoNothingInstanceStopped(bool) {}

chromeos::SessionManagerClient* GetSessionManagerClient() {
  // If the DBusThreadManager or the SessionManagerClient aren't available,
  // there isn't much we can do. This should only happen when running tests.
  if (!chromeos::DBusThreadManager::IsInitialized() ||
      !chromeos::DBusThreadManager::Get())
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

ArcStopReason GetStopReasonForStartFailure(StartArcInstanceResult result,
                                           bool stop_requested) {
  if (stop_requested)
    return ArcStopReason::SHUTDOWN;

  switch (result) {
    case StartArcInstanceResult::LOW_FREE_DISK_SPACE:
      return ArcStopReason::LOW_DISK_SPACE;
    case StartArcInstanceResult::UNKNOWN_ERROR:
      return ArcStopReason::GENERIC_BOOT_FAILURE;

    // Should never be reached. Fall through to NOTREACHED.
    case StartArcInstanceResult::SUCCESS: {
    }
  }
  NOTREACHED();
  return ArcStopReason::GENERIC_BOOT_FAILURE;
}

// TODO(hidehiko): Refactor more to make this class unittest-able, for at least
// state-machine part.
class ArcSessionImpl : public ArcSession,
                       public chromeos::SessionManagerClient::Observer {
 public:
  // The sequence to start an instance can be following three.
  //
  // 1) From NOT_STARTED to RUNNING_MINI_INSTANCE.
  // NOT_STARTED
  //   -> StartMiniInstance() ->
  // STARTING_MINI_INSTANCE
  //   -> OnMiniInstanceStarted() ->
  // RUNNING_MINI_INSTANCE
  //
  // 2) From NOT_STARTED to RUNNING
  // NOT_STARTED
  //   -> Start() ->
  // STARTING_INSTANCE
  //   -> OnInstanceStarted() ->
  // CONNECTING_MOJO
  //   -> ConnectMojo() -> OnMojoConnected() ->
  // RUNNING
  //
  // 3) From RUNNING_MINI_INSTANCE to RUNNING
  // RUNNING_MINI_INSTANCE
  //   -> Start() ->
  // STARTING_INSTANCE
  //   -> OnInstanceStarted() ->
  // CONNECTING_MOJO
  //   -> ConnectMojo() -> OnMojoConnected() ->
  // RUNNING
  //
  // Note that 2) is effectively the short-cut path of 1) + 3).
  // cf) b/65548422.
  // Also, note that Start() can be called during STARTING_MINI_INSTANCE.
  // Then, on its success callback, it automatically starts the sequence 3).
  //
  // Regardless of whether the instance is for login screen or not, at any
  // state, Stop() can be called. It may not immediately stop the instance,
  // but will eventually stop it. The actual stop will be notified via
  // ArcSession::Observer::OnSessionStopped().
  //
  // When Stop() is called, it makes various behavior based on the current
  // phase.
  //
  // NOT_STARTED:
  //   Do nothing. Immediately transition to the STOPPED state.
  // STARTING_INSTANCE, STARTING_MINI_INSTANCE:
  //   The ARC instance is starting via SessionManager. Stop() just sets the
  //   flag and return. On the main task completion, a callback will run on the
  //   thread, and the flag is checked. This should work under the assumption
  //   that the main tasks do not block indefinitely.
  //   In its callback, it checks if ARC instance is successfully started or
  //   not. In case of success, a request to stop the ARC instance is sent to
  //   SessionManager. Its completion will be notified via ArcInstanceStopped.
  //   Otherwise, it just turns into STOPPED state.
  // CONNECTING_MOJO:
  //   The main task runs on TaskScheduler's thread, but it is a blocking call.
  //   So, Stop() sends a request to cancel the blocking by closing the pipe
  //   whose read side is also polled. Then, in its callback, similar to
  //   STARTING_INSTANCE, a request to stop the ARC instance is sent to
  //   SessionManager, and ArcInstanceStopped handles remaining procedure.
  // RUNNING, RUNNING_MINI_INSTANCE:
  //   There is no more callback which runs on normal flow, so Stop() requests
  //   to stop the ARC instance via SessionManager.
  //
  // Another trigger to change the state coming from outside of this class
  // is an event ArcInstanceStopped() sent from SessionManager, when ARC
  // instace unexpectedly terminates. ArcInstanceStopped() turns the state into
  // STOPPED immediately.
  // If this happens during CONNECTING_MOJO, we expect the connection will not
  // be established, and its callback will be called if this instance is alive.
  //
  // In NOT_STARTED or STOPPED state, the instance can be safely destructed.
  // Specifically, in STOPPED state, there may be inflight operations or
  // pending callbacks. Though, what they do is just do-nothing conceptually
  // and they can be safely ignored.
  //
  // Note: Order of constants below matters. Please make sure to sort them
  // in chronological order.
  enum class State {
    // ARC is not yet started.
    NOT_STARTED,

    // The request to start an mini instance has been sent.
    STARTING_MINI_INSTANCE,

    // The instance is set up, but only a handful of processes NOT including
    // arcbridgeservice (i.e. mojo endpoint) are running.
    RUNNING_MINI_INSTANCE,

    // The request to start or upgrade the instance has been sent.
    STARTING_INSTANCE,

    // The instance has started. Waiting for it to connect to the IPC bridge.
    CONNECTING_MOJO,

    // The instance is fully set up.
    RUNNING,

    // ARC is terminated.
    STOPPED,
  };

  explicit ArcSessionImpl(ArcBridgeService* arc_bridge_service);
  ~ArcSessionImpl() override;

  // ArcSession overrides:
  void StartMiniInstance() override;
  void Start() override;
  bool IsStartRequested() override;
  void Stop() override;
  void OnShutdown() override;

 private:
  // D-Bus callback for StartArcInstance() to start mini instance.
  void OnMiniInstanceStarted(StartArcInstanceResult result,
                             const std::string& container_instance_id,
                             base::ScopedFD socket_fd);

  // Starts a fully features instance. On completion, OnInstanceStarted()
  // is called.
  void StartInternal();

  // D-Bus callback for StartArcInstance().
  void OnInstanceStarted(StartArcInstanceResult result,
                         const std::string& container_instance_id,
                         base::ScopedFD socket_fd);

  // Starts to connect ArcBridgeService Mojo channel.
  void ConnectMojo(base::ScopedFD socket_fd);

  // Synchronously accepts a connection on |socket_fd| and then processes the
  // connected socket's file descriptor.
  static mojo::ScopedMessagePipeHandle AcceptMojoConnection(
      mojo::edk::ScopedPlatformHandle socket_fd,
      base::ScopedFD cancel_fd);

  // Called when Mojo channel is accept(2)'ed.
  void OnMojoConnected(mojo::ScopedMessagePipeHandle server_pipe);

  // Request to stop ARC instance via DBus.
  void StopArcInstance();

  // chromeos::SessionManagerClient::Observer:
  void ArcInstanceStopped(bool clean,
                          const std::string& container_instance_id) override;

  // Completes the termination procedure. Note that calling this may end up with
  // deleting |this| because the function calls observers' OnSessionStopped().
  void OnStopped(ArcStopReason reason);

  // Checks whether a function runs on the thread where the instance is
  // created.
  THREAD_CHECKER(thread_checker_);

  // Owned by ArcServiceManager.
  ArcBridgeService* const arc_bridge_service_;

  // The state of the session.
  State state_ = State::NOT_STARTED;

  // When Start() is called, this flag is set;
  bool start_requested_ = false;

  // When Stop() is called, this flag is set.
  bool stop_requested_ = false;

  // Container instance id passed from session_manager.
  // Should be available only after OnInstanceStarted().
  std::string container_instance_id_;

  // In CONNECTING_MOJO state, this is set to the write side of the pipe
  // to notify cancelling of the procedure.
  base::ScopedFD accept_cancel_pipe_;

  // Mojo endpoint.
  std::unique_ptr<mojom::ArcBridgeHost> arc_bridge_host_;

  // WeakPtrFactory to use callbacks.
  base::WeakPtrFactory<ArcSessionImpl> weak_factory_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcSessionImpl);
};

ArcSessionImpl::ArcSessionImpl(ArcBridgeService* arc_bridge_service)
    : arc_bridge_service_(arc_bridge_service), weak_factory_(this) {
  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  if (client == nullptr)
    return;
  client->AddObserver(this);
}

ArcSessionImpl::~ArcSessionImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(state_ == State::NOT_STARTED || state_ == State::STOPPED);
  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  if (client == nullptr)
    return;
  client->RemoveObserver(this);
}

void ArcSessionImpl::StartMiniInstance() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(State::NOT_STARTED, state_);

  VLOG(2) << "Starting mini ARC instance";
  state_ = State::STARTING_MINI_INSTANCE;

  GetSessionManagerClient()->StartArcInstance(
      chromeos::SessionManagerClient::ArcStartupMode::LOGIN_SCREEN,
      // All variables below except |cb| will be ignored.
      cryptohome::Identification(), false, false,
      base::FeatureList::IsEnabled(arc::kNativeBridgeExperimentFeature),
      base::Bind(&ArcSessionImpl::OnMiniInstanceStarted,
                 weak_factory_.GetWeakPtr()));
}

void ArcSessionImpl::OnMiniInstanceStarted(
    StartArcInstanceResult result,
    const std::string& container_instance_id,
    base::ScopedFD socket_fd) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, State::STARTING_MINI_INSTANCE);
  DCHECK(!container_instance_id.empty());
  container_instance_id_ = container_instance_id;

  if (result != StartArcInstanceResult::SUCCESS) {
    LOG(ERROR) << "Failed to start ARC mini instance";
    OnStopped(GetStopReasonForStartFailure(result, stop_requested_));
    return;
  }

  VLOG(2) << "ARC mini instance is successfully started.";
  state_ = State::RUNNING_MINI_INSTANCE;

  if (stop_requested_) {
    StopArcInstance();
    return;
  }

  // If Start() is called during the mini instance start up, then upgrade it
  // to full instance.
  if (start_requested_) {
    VLOG(2) << "Resuming an existing ARC instance";
    StartInternal();
  }
}

void ArcSessionImpl::Start() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Start() can be called either for starting ARC from scratch or for
  // resuming an existing one. Start() must be able to start a fully
  // functional instance from all of |state_| up to and including
  // RUNNING_MINI_INSTANCE.
  DCHECK_GE(State::RUNNING_MINI_INSTANCE, state_);
  start_requested_ = true;

  if (state_ == State::STARTING_MINI_INSTANCE) {
    VLOG(2) << "Requested to resume an existing ARC instance";
    // OnStartMiniInstance() will start a fully featured instance.
    return;
  }

  VLOG(2) << (state_ == State::NOT_STARTED
                  ? "Starting ARC session"
                  : "Resuming an existing ARC instance");
  StartInternal();
}

void ArcSessionImpl::StartInternal() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // This can be called for starting the full instance from scratch,
  // or upgrading an already started mini instance.
  DCHECK(state_ == State::NOT_STARTED ||
         state_ == State::RUNNING_MINI_INSTANCE);
  DCHECK(start_requested_);
  state_ = State::STARTING_INSTANCE;

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

  GetSessionManagerClient()->StartArcInstance(
      chromeos::SessionManagerClient::ArcStartupMode::FULL, cryptohome_id,
      skip_boot_completed_broadcast, scan_vendor_priv_app,
      base::FeatureList::IsEnabled(arc::kNativeBridgeExperimentFeature),
      base::Bind(&ArcSessionImpl::OnInstanceStarted,
                 weak_factory_.GetWeakPtr()));
}

void ArcSessionImpl::OnInstanceStarted(StartArcInstanceResult result,
                                       const std::string& container_instance_id,
                                       base::ScopedFD socket_fd) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, State::STARTING_INSTANCE);

  if (result != StartArcInstanceResult::SUCCESS) {
    LOG(ERROR) << "Failed to start ARC instance";
    OnStopped(GetStopReasonForStartFailure(result, stop_requested_));
    return;
  }

  // Exclusively either container_instance_id_ or container_instance_id
  // needs to be empty.
  DCHECK_NE(container_instance_id_.empty(), container_instance_id.empty());
  if (container_instance_id_.empty()) {
    container_instance_id_ = container_instance_id;
  }

  VLOG(2) << "ARC instance is successfully "
          << (container_instance_id.empty() ? "resumed" : "started") << ".";

  if (stop_requested_) {
    StopArcInstance();
    return;
  }

  ConnectMojo(std::move(socket_fd));
}

void ArcSessionImpl::ConnectMojo(base::ScopedFD socket_fd) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, State::STARTING_INSTANCE);

  VLOG(2) << "Connecting Mojo...";
  state_ = State::CONNECTING_MOJO;

  // Prepare a pipe so that AcceptInstanceConnection can be interrupted on
  // Stop().
  base::ScopedFD cancel_fd;
  if (!CreatePipe(&cancel_fd, &accept_cancel_pipe_)) {
    StopArcInstance();
    return;
  }

  // For production, |socket_fd| passed from session_manager is either a valid
  // socket or a valid file descriptor (/dev/null). For testing, |socket_fd|
  // might be invalid.
  mojo::edk::PlatformHandle raw_handle(socket_fd.release());
  raw_handle.needs_connection = true;

  mojo::edk::ScopedPlatformHandle mojo_socket_fd(raw_handle);
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ArcSessionImpl::AcceptMojoConnection,
                     std::move(mojo_socket_fd), std::move(cancel_fd)),
      base::BindOnce(&ArcSessionImpl::OnMojoConnected,
                     weak_factory_.GetWeakPtr()));
}

// static
mojo::ScopedMessagePipeHandle ArcSessionImpl::AcceptMojoConnection(
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

void ArcSessionImpl::OnMojoConnected(
    mojo::ScopedMessagePipeHandle server_pipe) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // This may be called after ARC instance has been stopped, e.g. when it is
  // crashed during CONNECTING_MOJO, and ArcInstanceStopped() is called.
  DCHECK(state_ == State::CONNECTING_MOJO || state_ == State::STOPPED);
  accept_cancel_pipe_.reset();
  if (state_ == State::STOPPED)
    return;

  if (stop_requested_) {
    StopArcInstance();
    return;
  }

  if (!server_pipe.is_valid()) {
    LOG(ERROR) << "Invalid pipe";
    StopArcInstance();
    return;
  }

  mojom::ArcBridgeInstancePtr instance;
  instance.Bind(mojo::InterfacePtrInfo<mojom::ArcBridgeInstance>(
      std::move(server_pipe), 0u));
  arc_bridge_host_ = base::MakeUnique<ArcBridgeHostImpl>(arc_bridge_service_,
                                                         std::move(instance));

  VLOG(2) << "Mojo is connected. ARC is running.";
  state_ = State::RUNNING;
  for (auto& observer : observer_list_)
    observer.OnSessionReady();
}

void ArcSessionImpl::Stop() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(2) << "Stopping ARC session is requested.";

  // For second time or later, just do nothing.
  // It is already in the stopping phase.
  if (stop_requested_)
    return;

  stop_requested_ = true;
  arc_bridge_host_.reset();
  switch (state_) {
    case State::NOT_STARTED:
      OnStopped(ArcStopReason::SHUTDOWN);
      return;

    case State::STARTING_INSTANCE:
    case State::STARTING_MINI_INSTANCE:
      // Before starting the ARC instance, we do nothing here.
      // At some point, a callback will be invoked on UI thread,
      // and stopping procedure will be run there.
      // On Chrome shutdown, it is not the case because the message loop is
      // already stopped here. Practically, it is not a problem because;
      // - On starting instance, the container instance can be leaked.
      // Practically it is not problematic because the session manager will
      // clean it up.
      return;

    case State::CONNECTING_MOJO:
      // Mojo connection is being waited on TaskScheduler's thread.
      // Request to cancel it. Following stopping procedure will run
      // in its callback.
      accept_cancel_pipe_.reset();
      return;

    case State::RUNNING:
    case State::RUNNING_MINI_INSTANCE:
      // Now ARC (mini) instance is running. Request to stop it.
      StopArcInstance();
      return;

    case State::STOPPED:
      // The instance is already stopped. Do nothing.
      return;
  }
}

void ArcSessionImpl::StopArcInstance() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_NE(state_, State::NOT_STARTED);
  DCHECK_NE(state_, State::STOPPED);

  VLOG(2) << "Requesting session_manager to stop ARC instance";

  // When the instance is not for login screen, change the |state_| in
  // ArcInstanceStopped().
  GetSessionManagerClient()->StopArcInstance(
      base::Bind(&DoNothingInstanceStopped));
}

void ArcSessionImpl::ArcInstanceStopped(
    bool clean,
    const std::string& container_instance_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (container_instance_id != container_instance_id_) {
    VLOG(1) << "Container instance id mismatch. Do nothing."
            << container_instance_id << " vs " << container_instance_id_;
    return;
  }

  VLOG(1) << "Notified that ARC instance is stopped "
          << (clean ? "cleanly" : "uncleanly");

  // Release |container_instance_id_| to avoid duplicate invocation situation.
  container_instance_id_.clear();

  // In case that crash happens during before the Mojo channel is connected,
  // unlock the TaskScheduler's thread.
  accept_cancel_pipe_.reset();

  ArcStopReason reason;
  if (stop_requested_) {
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

bool ArcSessionImpl::IsStartRequested() {
  return !start_requested_;
}

void ArcSessionImpl::OnStopped(ArcStopReason reason) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // OnStopped() should be called once per instance.
  DCHECK_NE(state_, State::STOPPED);
  VLOG(2) << "ARC session is stopped.";
  arc_bridge_host_.reset();
  state_ = State::STOPPED;
  for (auto& observer : observer_list_)
    observer.OnSessionStopped(reason);
}

void ArcSessionImpl::OnShutdown() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  stop_requested_ = true;
  if (state_ == State::STOPPED)
    return;

  // Here, the message loop is already stopped, and the Chrome will be soon
  // shutdown. Thus, it is not necessary to take care about restarting case.
  // If ArcSession is waiting for mojo connection, cancels it.
  accept_cancel_pipe_.reset();

  // Stops the ARC instance to let it graceful shutdown.
  // Note that this may fail if ARC container is not actually running, but
  // ignore an error as described below.
  StopArcInstance();

  // Directly set to the STOPPED state by OnStopped(). Note that calling
  // StopArcInstance() may not work well. At least, because the UI thread is
  // already stopped here, ArcInstanceStopped() callback cannot be invoked.
  OnStopped(ArcStopReason::SHUTDOWN);
}

}  // namespace

ArcSession::ArcSession() = default;
ArcSession::~ArcSession() = default;

void ArcSession::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void ArcSession::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

// static
std::unique_ptr<ArcSession> ArcSession::Create(
    ArcBridgeService* arc_bridge_service) {
  return base::MakeUnique<ArcSessionImpl>(arc_bridge_service);
}

}  // namespace arc
