// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_ACTIVE_ACTIVATOR_TRACKER_H_
#define UI_AURA_KEYBOARD_LOCK_ACTIVE_ACTIVATOR_TRACKER_H_

#include "base/logging.h"
#include "ui/aura/keyboard_lock/activator.h"
#include "ui/aura/keyboard_lock/state_keeper_collection.h"

namespace ui {

class KeyEventFilter;

namespace aura {
namespace keyboard_lock {

template <typename CLIENT_ID>
class ActiveClientTracker final {
 public:
  explicit ActiveClientTracker(StateKeeperCollection<CLIENT_ID>* collection)
      : collection_(collection) {
    DCHECK(collection_);
  }

  ~ActiveClientTracker() = default;

  // Activates the reserved keys for |client| and marks it as active. So the
  // reserved keys of |client| will be registered eventually to PlatformHook,
  // and the keys received from PlatformHook::filter() will be sent to |client|.
  bool Activate(const CLIENT_ID& client) {
    client_ = client;
    inactive_ = false;
    Activator* activator = collection_->Find(client);
    if (activator) {
      activator->Activate(base::Callback<void(bool)>());
      return true;
    }
    LOG(ERROR) << "Cannot find StateKeeper of " << (size_t)client;
    return false;
  }

  // Deactivates |client| and marks it as deactive. So the reserved keys of
  // |client| will be unregistered eventually from PlatformHook, and the keys
  // received from PlatformHook::filter() will not be sent to |client|.
  bool Deactivate(const CLIENT_ID& client) {
    Activator* activator = collection_->Find(client);
    if (!activator) {
      LOG(ERROR) << "Cannot find StateKeeper of " << (size_t)client;
      return false;
    }
    if (client_ == client) {
      inactive_ = true;
      activator->Deactivate(base::Callback<void(bool)>());
    }
    return true;
  }

  // Returns the KeyEventFilter associated with the active client.
  KeyEventFilter* GetActiveKeyEventFilter() const {
    if (inactive_) {
      return nullptr;
    }
    return collection_->Find(client_);
  }

 private:
  StateKeeperCollection<CLIENT_ID>* const collection_;
  bool inactive_ = true;
  CLIENT_ID client_;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_ACTIVE_ACTIVATOR_TRACKER_H_
