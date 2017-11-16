// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/mojo_connection_notifier.h"

#include "components/arc/mojo_connection_observer.h"

namespace arc {
namespace internal {

MojoConnectionNotifier::MojoConnectionNotifier() = default;

MojoConnectionNotifier::~MojoConnectionNotifier() = default;

void MojoConnectionNotifier::AddObserver(MojoConnectionObserverBase* observer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  observer_list_.AddObserver(observer);
}

void MojoConnectionNotifier::RemoveObserver(
    MojoConnectionObserverBase* observer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  observer_list_.RemoveObserver(observer);
}

void MojoConnectionNotifier::NotifyConnectionReady() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  for (auto& observer : observer_list_)
    observer.OnConnectionReady();
}

void MojoConnectionNotifier::NotifyConnectionClosed() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  for (auto& observer : observer_list_)
    observer.OnConnectionClosed();
}

}  // namespace internal
}  // namespace arc
