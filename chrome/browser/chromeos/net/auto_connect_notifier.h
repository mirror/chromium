// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NET_AUTO_CONNECT_NOTIFIER_H_
#define CHROME_BROWSER_CHROMEOS_NET_AUTO_CONNECT_NOTIFIER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chromeos/network/auto_connect_handler.h"

class Profile;

namespace chromeos {

// Notifies the user when a network has been auto-connected.
class AutoConnectNotifier : public AutoConnectHandler::Observer {
 public:
  AutoConnectNotifier(Profile* profile,
                      AutoConnectHandler* auto_connect_handler);
  virtual ~AutoConnectNotifier();

 protected:
  // AutoConnectHandler::Observer:
  void OnAutoConnectedToNetwork() override;

 private:
  Profile* profile_;
  AutoConnectHandler* auto_connect_handler_;

  DISALLOW_COPY_AND_ASSIGN(AutoConnectNotifier);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NET_AUTO_CONNECT_NOTIFIER_H_
