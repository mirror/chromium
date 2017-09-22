// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DBUS_ERROR_H_
#define DBUS_ERROR_H_

#include <string>

#include "dbus/dbus_export.h"

namespace dbus {

// Encapsulates D-Bus error information. Example use cases are;
// - Converted from DBusError.
//   DBusError::name and DBusError::message are corresponding to name and
//   message, respectively.
// - Converted from ErrorResponse.
//   Message::GetErrorName() and first string data are corresponding to
//   name and message, respectively.
// Note that this is copyable/movable like a simple std::pair.
class CHROME_DBUS_EXPORT Error {
 public:
  Error(const std::string& name, const std::string& message);
  ~Error();

  // Returns the error name. It looks like
  // "org.freedesktop.DBus.Error.ServiceUnknown" etc.
  const std::string& name() const { return name_; }

  // Returns the detailed error message.
  const std::string& message() const { return message_; }

 private:
  std::string name_;
  std::string message_;
};

}  // namespace dbus

#endif  // DEBUS_ERROR_H_
