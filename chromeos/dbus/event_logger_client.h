// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_EVENT_LOGGER_CLIENT_H_
#define CHROMEOS_DBUS_EVENT_LOGGER_CLIENT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"

namespace chromeos {

// EventLoggerClient is used to communicate with the com.ubuntu.EventLogger
// sevice. All methods should be called from the origin thread (UI thread) which
// initializes the DBusThreadManager instance.
class CHROMEOS_EXPORT EventLoggerClient : public DBusClient {
 public:
  ~EventLoggerClient() override;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static EventLoggerClient* Create();

  // Log event to Chrome OS, passing an arbitrary string as the name.
  virtual void LogEvent(const std::string& event_name) = 0;

 protected:
  // Create() should be used instead.
  EventLoggerClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(EventLoggerClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_EVENT_LOGGER_CLIENT_H_
