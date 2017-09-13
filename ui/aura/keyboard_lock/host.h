// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_HOST_H_
#define UI_AURA_KEYBOARD_LOCK_HOST_H_

#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "ui/aura/keyboard_lock/activator.h"
#include "ui/aura/keyboard_lock/active_client_key_event_filter.h"
#include "ui/aura/keyboard_lock/active_client_tracker.h"
#include "ui/aura/keyboard_lock/client.h"
#include "ui/aura/keyboard_lock/observer.h"
#include "ui/aura/keyboard_lock/state_keeper_collection.h"
#include "ui/aura/keyboard_lock/types.h"
#include "ui/events/keycodes/dom/keycode_converter.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

class PlatformHook;

// A host of the keyboard lock feature. It provides a set of functions to let
// clients reserve keys and maintain the lifetime of clients.
// |CLIENT_ID| indicates the client, Host uses it to decide whether a new Client
// instance should be created when a request coming.
template <typename CLIENT_ID, typename PLATFORM_HOOK>
class Host {
 public:
  Host(scoped_refptr<base::SingleThreadTaskRunner> runner)
      : runner_(runner),
        tracker_(&state_keepers_),
        active_key_event_filter_(tracker_),
        platform_hook_(&active_key_event_filter_) {
    DCHECK(runner_);
  }

  virtual ~Host() = default;

  void Reserve(const CLIENT_ID& client,
               const std::vector<std::string>& codes,
               base::Callback<void(bool)> on_result) {
    std::vector<NativeKeyCode> native_codes;
    for (const std::string& code : codes) {
      native_codes.push_back(KeycodeConverter::CodeStringToNativeKeycode(code));
    }
    ReserveCodes(client, native_codes, on_result);
  }

  void ClearReserved(const CLIENT_ID& client,
                     base::Callback<void(bool)> on_result) {
    // TODO(zijiehe): Should we notify the cooresponding |observer| immediately?
    if (!runner_->BelongsToCurrentThread()) {
      if (!runner_->PostTask(FROM_HERE, base::BindOnce(
              &Host::ClearReserved,
              base::Unretained(this),
              client,
              on_result))) {
        if (on_result) {
          on_result.Run(false);
        }
      }
      return;
    }

    state_keepers_.Erase(client);
  }

 private:
  void ReserveCodes(const CLIENT_ID& client,
                    const std::vector<NativeKeyCode>& codes,
                    base::Callback<void(bool)> on_result) {
    if (!runner_->BelongsToCurrentThread()) {
      if (!runner_->PostTask(FROM_HERE, base::BindOnce(
              &Host::ReserveCodes,
              base::Unretained(this),
              client,
              codes,
              on_result))) {
        if (on_result) {
          on_result.Run(false);
        }
      }
      return;
    }

    StateKeeper* state_keeper = state_keepers_.Find(client);
    if (state_keeper) {
      state_keeper->Register(codes, on_result);
      return;
    }

    std::unique_ptr<StateKeeper> new_state_keeper =
        base::MakeUnique<StateKeeper>(CreateClient(client),
                                      &platform_hook_);
    new_state_keeper->Register(codes, on_result);
    state_keepers_.Insert(client, std::move(new_state_keeper));
    std::unique_ptr<Observer<CLIENT_ID>> observer =
        base::MakeUnique<Observer<CLIENT_ID>>(&state_keepers_,
                                              &tracker_,
                                              client);
    ObserveClient(client, std::move(observer));
    DCHECK(state_keeper);
  }

  // Let the derived class create a scenario depedent Client.
  virtual std::unique_ptr<Client> CreateClient(const CLIENT_ID& client) = 0;

  // Let the derived class observe |client| and report its state back from
  // |observer|.
  virtual void ObserveClient(const CLIENT_ID& client,
                             std::unique_ptr<Observer<CLIENT_ID>> observer) = 0;

  const scoped_refptr<base::SingleThreadTaskRunner> runner_;
  StateKeeperCollection<CLIENT_ID> state_keepers_;
  ActiveClientTracker<CLIENT_ID> tracker_;
  ActiveClientKeyEventFilter<CLIENT_ID> active_key_event_filter_;
  PLATFORM_HOOK platform_hook_;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_HOST_H_
