// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_SYSTEM_CLOCK_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_SYSTEM_CLOCK_CLIENT_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chromeos/dbus/system_clock_client.h"

namespace chromeos {

// A fake implementation of SystemClockClient. This class does nothing.
class CHROMEOS_EXPORT FakeSystemClockClient : public SystemClockClient {
 public:
  FakeSystemClockClient();
  ~FakeSystemClockClient() override;

  // SystemClockClient overrides
  void Init(dbus::Bus* bus) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  bool HasObserver(const Observer* observer) const override;
  void SetTime(int64_t time_in_seconds) override;
  bool CanSetTime() override;
  void GetLastSyncInfo(const GetLastSyncInfoCallback& callback) override;

  void NotifyObserversSystemClockUpdated();

  void set_network_synchronized(bool network_synchronized) {
    network_synchronized_ = network_synchronized;
  }

 private:
  void OnGetLastSyncInfo(const GetLastSyncInfoCallback& callback);

  bool network_synchronized_ = false;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<FakeSystemClockClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakeSystemClockClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_SYSTEM_CLOCK_CLIENT_H_
