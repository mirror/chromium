// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_STATE_KEEPER_COLLECTION_H_
#define UI_AURA_KEYBOARD_LOCK_STATE_KEEPER_COLLECTION_H_

#include <map>
#include <memory>
#include <utility>

#include "base/logging.h"
#include "ui/aura/keyboard_lock/state_keeper.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// A mapping between clients and their state_keepers.
template <typename CLIENT_ID>
class StateKeeperCollection final {
 public:
  StateKeeperCollection() = default;
  ~StateKeeperCollection() = default;

  StateKeeper* Find(const CLIENT_ID& client) const {
    auto it = state_keepers_.find(client);
    if (it == state_keepers_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  void Insert(const CLIENT_ID& client,
              std::unique_ptr<StateKeeper> state_keeper) {
    DCHECK(state_keeper);
    auto result =
        state_keepers_.insert(std::make_pair(client, std::move(state_keeper)));
    DCHECK(result.second);
  }

  void Erase(const CLIENT_ID& client) {
    state_keepers_.erase(client);
  }

 private:
  std::map<const CLIENT_ID, std::unique_ptr<StateKeeper>> state_keepers_;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_STATE_KEEPER_COLLECTION_H_
