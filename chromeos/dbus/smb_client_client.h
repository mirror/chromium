// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMIUM_SMBCLIENT_CLIENT_H
#define CHROMIUM_SMBCLIENT_CLIENT_H

#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

struct EntryData {
  bool is_directory;
  std::string name;
  std::string fullPath;
  int64_t size;
  int64_t modification_time;
};

// using base::Callback<void(DBusMethodCallStatus call_status, std::vector<int>
// entries)> StringVectorDBusMethodCallback;
using StringVectorDBusMethodCallback =
    base::Callback<void(DBusMethodCallStatus call_status,
                        std::vector<std::string> entries)>;

using EntryDataMethodCallback =
    base::Callback<void(DBusMethodCallStatus call_status,
                        std::vector<EntryData> readDirResult)>;

using FileDescriptorMethodCallback = base::Callback < vo

                                     class CHROMEOS_EXPORT SmbClientClient
    : public DBusClient {
 public:
  ~SmbClientClient() override;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static SmbClientClient* Create();

  // Stub method to communicate to daemon service
  virtual void CommunicateToService(const std::string& directoryPath,
                                    EntryDataMethodCallback callback) = 0;

 protected:
  // Create() should be used instead.
  SmbClientClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(SmbClientClient);
};

}  // namespace chromeos

#endif  // CHROMIUM_SMBCLIENT_CLIENT_H
