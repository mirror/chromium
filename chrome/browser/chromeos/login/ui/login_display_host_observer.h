// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_UI_LOGIN_DISPLAY_HOST_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_UI_LOGIN_DISPLAY_HOST_OBSERVER_H_

#include "base/macros.h"

namespace chromeos {

// Notifies about life-cycle events of LoginDisplayHost.
class LoginDisplayHostObserver {
 public:
  // Called when LoginDisplayHost is closed.
  virtual void OnLoginDisplayHostClosed() = 0;

 protected:
  virtual ~LoginDisplayHostObserver() {}
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_UI_LOGIN_DISPLAY_HOST_OBSERVER_H_
