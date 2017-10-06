// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_HUDDLY_MONITOR_CLIENT_H_
#define CHROMEOS_DBUS_HUDDLY_MONITOR_CLIENT_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_client_implementation_type.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

class CHROMEOS_EXPORT HuddlyMonitorClient : public DBusClient {
 public:

  virtual void UpdateCondition(bool updated_condition, uint64_t timestamp);

  ~HuddlyMonitorClient() override;

  static HuddlyMonitorClient* Create();
 protected:
  // Create() should be used instead.
  HuddlyMonitorClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(HuddlyMonitorClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_HUDDLY_MONITOR_CLIENT_H_
