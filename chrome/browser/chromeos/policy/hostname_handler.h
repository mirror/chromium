// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_HOSTNAME_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_HOSTNAME_HANDLER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"

namespace policy {

// This class observes the device setting |DeviceHostname|, and calls
// NetworkStateHandler::SetHostname() appropriately based on the value of that
// setting.
class HostnameHandler {
 public:
  explicit HostnameHandler(chromeos::CrosSettings* cros_settings);
  ~HostnameHandler();

 private:
  void OnDeviceHostnamePropertyChanged();

  chromeos::CrosSettings* cros_settings_;
  std::unique_ptr<chromeos::CrosSettings::ObserverSubscription>
      policy_subscription_;
  base::WeakPtrFactory<HostnameHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HostnameHandler);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_HOSTNAME_HANDLER_H_
