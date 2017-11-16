// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_MOJO_CONNECTION_NOTIFIER_H_
#define COMPONENTS_ARC_MOJO_CONNECTION_NOTIFIER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"

namespace arc {
namespace internal {

class MojoConnectionObserverBase;

// Manages events related to Mojo connection. Designed to be used only
// by InstanceHolder.
class MojoConnectionNotifier {
 public:
  MojoConnectionNotifier();
  ~MojoConnectionNotifier();

  void AddObserver(MojoConnectionObserverBase* observer);
  void RemoveObserver(MojoConnectionObserverBase* observer);

  // Notifies observers that connection gets ready.
  void NotifyConnectionReady();

  // Notifies observers that connection is closed.
  void NotifyConnectionClosed();

 private:
  THREAD_CHECKER(thread_checker_);
  base::ObserverList<MojoConnectionObserverBase> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(MojoConnectionNotifier);
};

}  // namespace internal
}  // namespace arc

#endif  // COMPONENTS_ARC_MOJO_CONNECTION_NOTIFIER_H_
