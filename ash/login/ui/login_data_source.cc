// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_data_source.h"

namespace ash {

LoginDataSource::Observer::Observer() {}

LoginDataSource::Observer::~Observer() {}

void LoginDataSource::Observer::OnUsersChanged(
    const std::vector<ash::mojom::UserInfoPtr>& users) {}

void LoginDataSource::Observer::SetPinEnabledForUser(const AccountId& user,
                                                     bool enabled) {}

LoginDataSource::LoginDataSource() = default;

LoginDataSource::~LoginDataSource() = default;

void LoginDataSource::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void LoginDataSource::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void LoginDataSource::NotifyUsers(
    const std::vector<ash::mojom::UserInfoPtr>& users) {
  for (auto& observer : observers_)
    observer.OnUsersChanged(users);
}

void LoginDataSource::SetPinEnabledForUser(const AccountId& user,
                                           bool enabled) {
  for (auto& observer : observers_)
    observer.SetPinEnabledForUser(user, enabled);
}

}  // namespace ash
