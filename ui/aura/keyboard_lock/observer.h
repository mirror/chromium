// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_OBSERVER_H_
#define UI_AURA_KEYBOARD_LOCK_OBSERVER_H_

#include "base/logging.h"
#include "ui/aura/keyboard_lock/active_client_tracker.h"
#include "ui/aura/keyboard_lock/state_keeper_collection.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// Receives activated and deactived events of a CLIENT_ID.
template <typename CLIENT_ID>
class Observer final {
 public:
  explicit Observer(StateKeeperCollection<CLIENT_ID>* collection,
                    ActiveClientTracker<CLIENT_ID>* tracker,
                    const CLIENT_ID& client)
      : collection_(collection),
        tracker_(tracker),
        client_(client) {
    DCHECK(collection_);
    DCHECK(tracker_);
  }

  virtual ~Observer() = default;

  bool Activate() { return tracker_->Activate(client_); }
  bool Deactivate() { return tracker_->Deactivate(client_); }
  void Erase() {
    Deactivate();
    collection_->Erase(client_);
  }

 private:
  StateKeeperCollection<CLIENT_ID>* const collection_;
  ActiveClientTracker<CLIENT_ID>* const tracker_;
  const CLIENT_ID client_;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_OBSERVER_H_
