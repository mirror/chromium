// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_ARC_SESSION_RUNNER_H_
#define COMPONENTS_ARC_ARC_SESSION_RUNNER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chromeos/dbus/session_manager_client.h"
// TODO:cmtm the below include statement is a IWYU violation
// This is just to get components/arc/common/arc_bridge.mojom.h
// I'll change it to that after
#include "components/arc/arc_bridge_host_impl.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_stop_reason.h"

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

using StartArcInstanceResult =
    chromeos::SessionManagerClient::StartArcInstanceResult;

// Accept requests to start/stop ARC instance. Also supports automatic
// restarting on unexpected ARC instance crash.
class ArcSessionRunner : public chromeos::SessionManagerClient::Observer {
 public:
  // Observer to notify events across multiple ARC session runs.
  class Observer {
   public:
    // Called when ARC instance is stopped. If |restarting| is true, another
    // ARC session is being restarted (practically after certain delay).
    // Note: this is called once per ARC session, including unexpected
    // CRASH on ARC container, and expected SHUTDOWN of ARC triggered by
    // RequestStop(), so may be called multiple times for one RequestStart().
    virtual void OnSessionStopped(ArcStopReason reason, bool restarting) = 0;

    // Called when ARC session is stopped, but is being restarted automatically.
    // Unlike OnSessionStopped() with |restarting| == true, this is called
    // _after_ the container is actually created.
    virtual void OnSessionRestarting() = 0;

   protected:
    virtual ~Observer() = default;
  };

  explicit ArcSessionRunner(ArcBridgeService* arc_bridge_service);
  ~ArcSessionRunner() override;

  // Add/Remove an observer.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Starts the ARC service, then it will connect the Mojo channel. When the
  // bridge becomes ready, registered Observer's OnSessionReady() is called.
  void RequestStart();

  // Stops the ARC service.
  // TODO(yusukes): Remove the parameter.
  void RequestStop();

  // OnShutdown() should be called when the browser is shutting down. This can
  // only be called on the thread that this class was created on. We assume that
  // when this function is called, MessageLoop is no longer exists.
  void OnShutdown();

  // Returns whether currently ARC instance is running or stopped respectively.
  // Note that, both can return false at same time when, e.g., starting
  // or stopping ARC instance.
  bool IsRunning() const;
  bool IsStopped() const;

  // Returns whether LoginScreen instance is starting.
  bool IsLoginScreenInstanceStarting() const;

  // Normally, automatic restarting happens after a short delay. When testing,
  // however, we'd like it to happen immediately to avoid adding unnecessary
  // delays.
  void SetRestartDelayForTesting(const base::TimeDelta& restart_delay);

 private:
  // The possible states.  In the normal flow, the state changes in the
  // following sequence:
  //
  // STOPPED
  //   RequestStart() ->
  // STARTING
  //   OnSessionReady() ->
  // RUNNING
  //
  // The ArcSession state machine can be thought of being substates of
  // ArcBridgeService's STARTING state.
  // ArcBridgeService's state machine can be stopped at any phase.
  //
  // *
  //   RequestStop() ->
  // STOPPING
  //   OnSessionStopped() ->
  // STOPPED
  enum class State {
    // ARC instance is not currently running.
    STOPPED,

    // Request to start ARC instance is received. Starting an ARC instance.
    STARTING,

    // The instance has started. Waiting for it to connect to the IPC bridge.
    CONNECTING_MOJO,

    // ARC instance has finished initializing, and is now ready for interaction
    // with other services.
    RUNNING,

    // Request to stop ARC instance is recieved. Stopping the ARC instance.
    STOPPING,
  };

  enum class TargetState {
    STOP,
    MINI,
    FULL,
  };

  // Starts to run an ARC instance.
  void Start();

  // Stop an ARC instance.
  void Stop();

  // Restarts an ARC instance.
  void RestartArcSession();

  void ArcInstanceStopped(bool clean, const std::string& container_instance_id);

  // Completes the termination procedure.
  void OnStopped(ArcStopReason reason);

  // Sends a StartArcInstance D-Bus request to session_manager.
  static void SendStartArcInstanceDBusMessage(
      bool instance_is_for_login_screen,
      const chromeos::SessionManagerClient::StartArcInstanceCallback& cb);

  // DBus callback for StartArcInstance().
  void OnInstanceStarted(bool instance_is_for_login_screen,
                         StartArcInstanceResult result,
                         const std::string& container_instance_id,
                         base::ScopedFD socket_fd);

  // Synchronously accepts a connection on |socket_fd| and then processes the
  // connected socket's file descriptor.
  static mojo::ScopedMessagePipeHandle ConnectMojo(
      mojo::edk::ScopedPlatformHandle socket_fd,
      base::ScopedFD cancel_fd);
  void OnMojoConnected(mojo::ScopedMessagePipeHandle server_pipe);

  // chromeos::SessionManagerClient::Observer:
  void EmitLoginPromptVisibleCalled() override;

  THREAD_CHECKER(thread_checker_);

  // Observers for the ARC instance state change events.
  base::ObserverList<Observer> observer_list_;

  // The target state of the container
  TargetState target_state_ = TargetState::STOP;

  // Container instance id passed from session_manager.
  // Should be available only after OnInstanceStarted().
  std::string container_instance_id_;

  // Instead of immediately trying to restart the container, give it some time
  // to finish tearing down in case it is still in the process of stopping.
  base::TimeDelta restart_delay_;
  base::OneShotTimer restart_timer_;

  // Owned by ArcServiceManager.
  ArcBridgeService* const arc_bridge_service_;

  // Current runner's state.
  State state_ = State::STOPPED;

  // In CONNECTING_MOJO state, this is set to the write side of the pipe
  // to notify cancelling of the procedure.
  base::ScopedFD accept_cancel_pipe_;

  // Mojo endpoint.
  std::unique_ptr<mojom::ArcBridgeHost> arc_bridge_host_;

  // WeakPtrFactory to use callbacks.
  base::WeakPtrFactory<ArcSessionRunner> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcSessionRunner);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_ARC_SESSION_RUNNER_H_
