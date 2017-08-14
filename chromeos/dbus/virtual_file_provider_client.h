// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_VIRTUAL_FILE_PROVIDER_CLIENT_H_
#define CHROMEOS_DBUS_VIRTUAL_FILE_PROVIDER_CLIENT_H_

#include <stdint.h>

#include <string>

#include "base/callback_forward.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"

namespace chromeos {

// VirtualFileProviderClient is used to communicate with the VirtualFileProvider
// service. VirtualFileProvider service provides file descriptors which
// forward read requests to chrome. From the reading process's perspective, the
// file descriptor behaves like a regular file descriptor (unlike pipe, it
// supports seek), while actually there is no real file associated with it.
class CHROMEOS_EXPORT VirtualFileProviderClient : public DBusClient {
 public:
  using OpenFileCallback =
      base::OnceCallback<void(const std::string& id, base::ScopedFD fd)>;
  using ReadRequestHandler =
      base::RepeatingCallback<void(const std::string& id,
                                   int64_t offset,
                                   int64_t size,
                                   base::ScopedFD pipe_write_end)>;
  using ReleaseIDHandler = base::RepeatingCallback<void(const std::string& id)>;

  VirtualFileProviderClient();
  ~VirtualFileProviderClient() override;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static VirtualFileProviderClient* Create();

  // Creates a new file descriptor and returns it with a unique ID.
  // |size| will be used perform boundary check when FD is seeked.
  // When the FD is read, the read request is forwarded to the request handler.
  virtual void OpenFile(int64_t size, OpenFileCallback callback) = 0;

  // Sets the read request handler which receives the ID of the FD (created w/
  // OpenFile()) being read, offset, size, and a write end of a pipe with which
  // the data should be sent back to the service.
  virtual void SetReadRequestHandler(const ReadRequestHandler& handler) = 0;

  // Sets the release ID handler which receives the ID of the FD (created w/
  // OpenFile()) when it's being closed.
  virtual void SetReleaseIDHandler(const ReleaseIDHandler& handler) = 0;
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_VIRTUAL_FILE_PROVIDER_CLIENT_H_
