// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_ASYNC_SHUTDOWN_TASK_H_
#define CHROMEOS_COMPONENTS_TETHER_ASYNC_SHUTDOWN_TASK_H_

#include "base/macros.h"
#include "base/observer_list.h"

namespace chromeos {

namespace tether {

// Performs asynchronous shutdown tasks after the Tether component has been
// destroyed. The Tether component is shut down synchronously, but there are
// some parts of the component which cannot complete their shutdown flows
// synchronously.
//
// When the Tether component is destroyed, it returns an instance of this class;
// the client shutting down the component should hold onto the instance until
// the Observer function OnAsyncShutdownComplete() has been called, at which
// time the instance can be deleted.
class AsyncShutdownTask {
 public:
  class Observer {
   public:
    Observer() {}
    virtual ~Observer() {}

    virtual void OnAsyncShutdownComplete() {}
  };

  AsyncShutdownTask();
  virtual ~AsyncShutdownTask();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void NotifyAsyncShutdownComplete();

 private:
  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(AsyncShutdownTask);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_ASYNC_SHUTDOWN_TASK_H_
