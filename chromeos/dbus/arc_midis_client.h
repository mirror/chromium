// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_ARC_MIDIS_CLIENT_H_
#define CHROMEOS_DBUS_ARC_MIDIS_CLIENT_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"

namespace chromeos {

// ArcMidisClient is used to pass an fd to the midis daemon for the purposes
// of setting up a mojo channel. It is expected to be called once during browser
// initialization.
class CHROMEOS_EXPORT ArcMidisClient : public DBusClient {
 public:
  ~ArcMidisClient() override;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static ArcMidisClient* Create();

  // Bootstrap the mojo connection between Chrome and the MIDI service.
  // Should pass in the child end of the Mojo pipe.
  virtual void BootstrapMojoConnection(base::ScopedFD fd,
                                       VoidDBusMethodCallback callback) = 0;

 protected:
  // Create() should be used instead.
  ArcMidisClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcMidisClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_ARC_MIDIS_CLIENT_H_
