// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_system_clock_client.h"
#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"

namespace chromeos {

FakeSystemClockClient::FakeSystemClockClient() : weak_ptr_factory_(this) {}

FakeSystemClockClient::~FakeSystemClockClient() {
}

void FakeSystemClockClient::Init(dbus::Bus* bus) {
}

void FakeSystemClockClient::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeSystemClockClient::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool FakeSystemClockClient::HasObserver(const Observer* observer) const {
  return observers_.HasObserver(observer);
}

void FakeSystemClockClient::SetTime(int64_t time_in_seconds) {}

bool FakeSystemClockClient::CanSetTime() {
  return true;
}

void FakeSystemClockClient::GetLastSyncInfo(
    const GetLastSyncInfoCallback& callback) {
  base::MessageLoop::current()->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&FakeSystemClockClient::OnGetLastSyncInfo,
                                weak_ptr_factory_.GetWeakPtr(), callback));
}

void FakeSystemClockClient::NotifyObserversSystemClockUpdated() {
  for (auto& observer : observers_)
    observer.SystemClockUpdated();
}

void FakeSystemClockClient::OnGetLastSyncInfo(
    const GetLastSyncInfoCallback& callback) {
  callback.Run(network_synchronized_);
}

}  // namespace chromeos
