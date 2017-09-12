// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_ACTIVE_ACTIVATOR_TRACKER_H_
#define UI_AURA_KEYBOARD_LOCK_ACTIVE_ACTIVATOR_TRACKER_H_

#include "base/checks.h"
#include "ui/aura/keyboard_lock/state_keeper_collection.h"

namespace ui {
namespace aura {
namespace keybaord_lock {

class Activator;

template <typename CLIENT_ID>
class ActiveClientTracker final {
 public:
  explicit ActiveClientTracker(StateKeeperCollection* collection)
      : collection_(collection) {
    DCHECK(collection_);
  }

  ~ActiveClientTracker() = default;

  bool Activate(const CLIENT_ID& client) {
    client_ = client;
    inactive_ = false;
    Activator* activator = collection_->Find(client_);
    if (activator) {
      activator->Activate(base::Callback<void(bool)>());
      return true;
    }
    return false;
  }

  bool Deactive(const CLIENT_ID& client) {
    Activator* activator = collection_->Find(client_);
    if (!activator) {
      return false;
    }
    if (client_ == client) {
      inactive_ = true;
      activator->Deactivate(base::Callback<void(bool)>());
    }
    return true;
  }

  KeyEventFilter* GetActiveKeyEventFilter() const {
    if (inactive_) {
      return nullptr;
    }
    return collection_->Find(client_);
  }

 private:
  StateKeeperCollection* const collection_;
  bool inactive_ = true;
  CLIENT_ID client_;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_ACTIVE_ACTIVATOR_TRACKER_H_
