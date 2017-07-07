// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_display_host.h"

#include "chrome/browser/chromeos/login/ui/login_display_host_observer.h"

namespace chromeos {

// static
LoginDisplayHost* LoginDisplayHost::default_host_ = nullptr;

LoginDisplayHost::LoginDisplayHost() = default;

LoginDisplayHost::~LoginDisplayHost() = default;

void LoginDisplayHost::AddObserver(LoginDisplayHostObserver* observer) {
  observer_list_.AddObserver(observer);
}

void LoginDisplayHost::RemoveObserver(LoginDisplayHostObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void LoginDisplayHost::NotifyClosed() {
  for (auto& observer : observer_list_)
    observer.OnLoginDisplayHostClosed();
}

}  // namespace chromeos
