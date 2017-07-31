// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_MIDIS_CLIENT_H_
#define CHROMEOS_DBUS_MIDIS_CLIENT_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "dbus/object_proxy.h"

namespace chromeos {

// MidisClient is used to pass an fd to the midis daemon for the purposes
// of setting up a mojo channel. It is expected to be called once during browser
// initialization.
class CHROMEOS_EXPORT MidisClient : public DBusClient {
 public:
  ~MidisClient() override;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static MidisClient* Create();

  // Calls GetAll method.  |callback| is called after the method call succeeds.
  virtual void BootstrapMojoConnection(const std::string& service_name,
                                       const dbus::ObjectPath& object_path) = 0;

 protected:
  // Create() should be used instead.
  MidisClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(MidisClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_MIDIS_CLIENT_H_
