// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_ARC_SESSION_H_
#define COMPONENTS_ARC_ARC_SESSION_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/task_runner.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_stop_reason.h"

namespace arc {

// Starts the ARC instance and bootstraps the bridge connection.
// Clients should implement the Delegate to be notified upon communications
// being available.
// The instance can be safely removed 1) before Start() is called, or 2) after
// OnSessionStopped() is called.
// The number of instances must be at most one. Otherwise, ARC instances will
// conflict.
class ArcSession {
 public:
  // Observer to notify events corresponding to one ARC session run.
  class Observer {
   public:
    // Called when the connection with ARC instance has been established.
    virtual void OnSessionReady() = 0;

    // Called when ARC instance is stopped. This is called exactly once
    // per instance which is Start()ed.
    virtual void OnSessionStopped(ArcStopReason reason) = 0;

   protected:
    virtual ~Observer() = default;
  };

  // Creates a default instance of ArcSession.
  static std::unique_ptr<ArcSession> Create(
      ArcBridgeService* arc_bridge_service,
      const scoped_refptr<base::TaskRunner>& blocking_task_runner);
  virtual ~ArcSession();

  // Starts an instance for login screen. The instance does not have any mojo
  // end point, and therefore OnSessionReady() will never be called. When
  // starting the container fails, OnSessionStopped() is NOT called either.
  virtual void StartForLoginScreen() = 0;

  // Stops an instance if it is for login screen. Thid does NOT call
  // OnSessionStopped().
  virtual void StopForLoginScreen() = 0;

  // Starts and bootstraps a connection with the instance. The Observer's
  // OnSessionReady() will be called if the bootstrapping is successful, or
  // OnSessionStopped() if it is not. Start() should not be called twice or
  // more. Calling Start() after calling StartForLoginScreen() is allowed.
  // Start() behaves the same regardless of whether StartForLoginScreen()
  // has already been called or not.
  virtual void Start() = 0;

  // Requests to stop the currently-running instance.
  // The completion is notified via OnSessionStopped() of the Observer.
  virtual void Stop() = 0;

  // Called when Chrome is in shutdown state. This is called when the message
  // loop is already stopped, and the instance will soon be deleted. Caller
  // may expect that OnSessionStopped() is synchronously called back except
  // when it has already been called before.
  virtual void OnShutdown() = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  ArcSession();

  base::ObserverList<Observer> observer_list_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcSession);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_ARC_SESSION_H_
