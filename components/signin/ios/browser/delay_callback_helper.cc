// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/ios/browser/delay_callback_helper.h"

DelayCallbackHelper::DelayCallbackHelper() {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

DelayCallbackHelper::~DelayCallbackHelper() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void DelayCallbackHelper::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (type >= net::NetworkChangeNotifier::ConnectionType::CONNECTION_NONE)
    return;

  for (const base::Closure& callback : delayed_callbacks_)
    callback.Run();

  delayed_callbacks_.clear();
}

void DelayCallbackHelper::HandleCallback(const base::Closure& callback) {
  // Save for later if we don't have any kind of network connection.
  if (net::NetworkChangeNotifier::IsOffline()) {
    delayed_callbacks_.push_back(callback);
  } else {
    callback.Run();
  }
}
