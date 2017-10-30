// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SMB_PROVIDER_CLIENT_H_
#define CHROMEOS_DBUS_SMB_PROVIDER_CLIENT_H_

#include <string>

#include "base/callback.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "dbus/object_proxy.h"

namespace chromeos {

// SmbProviderClient is used to communicate with the org.chromium.SmbProvider
// service. All methods should be called from the origin thread (UI thread)
// which initializes the DBusThreadManager instance.
class CHROMEOS_EXPORT SmbProviderClient : public DBusClient {
 public:
  using MountCallback =
      base::OnceCallback<void(int32_t error_code, int32_t mount_id)>;
  using UnmountCallback = base::OnceCallback<void(int32_t error_code)>;

  ~SmbProviderClient() override;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static SmbProviderClient* Create();

  // Calls Mount. It runs "OpenDirectory" on |share_path| to check that it is a
  // valid share. |callback| is called after getting (or failing to get) D-BUS
  // response.
  virtual void Mount(const std::string& share_path, MountCallback callback) = 0;

  // Calls Unmount. This removes the corresponding mount of |mount_id| from
  // being able to be accessed through SmbProvider.
  virtual void Unmount(int32_t mount_id, UnmountCallback callback) = 0;

 protected:
  // Create() should be used instead.
  SmbProviderClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(SmbProviderClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SMB_PROVIDER_CLIENT_H_
