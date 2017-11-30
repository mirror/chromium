// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_IOS_BROWSER_DELAY_CALLBACK_HELPER_H_
#define COMPONENTS_SIGNIN_IOS_BROWSER_DELAY_CALLBACK_HELPER_H_

#include <list>

#include "base/callback.h"
#include "base/macros.h"
#include "net/base/network_change_notifier.h"

class DelayCallbackHelper
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  DelayCallbackHelper();
  ~DelayCallbackHelper();

  // net::NetworkChangeController::NetworkChangeObserver implementation.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // Pass the |callback| that may need to be called later on.
  void HandleCallback(const base::Closure& callback);

 private:
  std::list<base::Closure> delayed_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(DelayCallbackHelper);
};

#endif  // COMPONENTS_SIGNIN_IOS_BROWSER_DELAY_CALLBACK_HELPER_H_
