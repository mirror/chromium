// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/smb_provider_client.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/weak_ptr.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

// TODO(allenvic): Temporary constants until we get access to dbus-constants.
namespace smbprovider {

// General
const char kSmbProviderInterface[] = "org.chromium.SmbProvider";
const char kSmbProviderServicePath[] = "/org/chromium/SmbProvider";
const char kSmbProviderServiceName[] = "org.chromium.SmbProvider";

// Methods
const char kMountMethod[] = "Mount";
const char kUnmountMethod[] = "Unmount";

}  // namespace smbprovider

namespace chromeos {

namespace {

int32_t GetErrorFromReader(dbus::MessageReader* reader) {
  int32_t int_error;
  if (!reader->PopInt32(&int_error)) {
    DLOG(ERROR)
        << "SmbProviderClient: Failed to get an error from the response";
    return -EIO;
  }
  return int_error;
}

class SmbProviderClientImpl : public SmbProviderClient {
 public:
  SmbProviderClientImpl() : weak_ptr_factory_(this) {}

  ~SmbProviderClientImpl() override {}

  void Mount(const std::string& share_path, MountCallback callback) override {
    dbus::MethodCall method_call(smbprovider::kSmbProviderInterface,
                                 smbprovider::kMountMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(share_path);
    proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::Bind(&SmbProviderClientImpl::HandleMountCallback,
                   weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback)));
  }

  void Unmount(int32_t mount_id, UnmountCallback callback) override {
    dbus::MethodCall method_call(smbprovider::kSmbProviderInterface,
                                 smbprovider::kUnmountMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendInt32(mount_id);
    proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::Bind(&SmbProviderClientImpl::HandleUnmountCallback,
                   weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback)));
  }

 protected:
  // DBusClient override.
  void Init(dbus::Bus* bus) override {
    proxy_ = bus->GetObjectProxy(
        smbprovider::kSmbProviderServiceName,
        dbus::ObjectPath(smbprovider::kSmbProviderServicePath));
  }

 private:
  // Handles DBus callback for mount.
  void HandleMountCallback(MountCallback callback, dbus::Response* response) {
    if (!response) {
      DLOG(ERROR) << "Mount: failed to call smbprovider";
      std::move(callback).Run(-EIO, -1);
      return;
    }
    dbus::MessageReader reader(response);
    int32_t error = GetErrorFromReader(&reader);
    if (error < 0) {
      std::move(callback).Run(error, -1);
      return;
    }
    int32_t mount_id;
    if (!reader.PopInt32(&mount_id)) {
      DLOG(ERROR) << "Mount: failed to parse mount id";
      std::move(callback).Run(-EIO, -1);
      return;
    }
    std::move(callback).Run(error, mount_id);
  }

  // Handles DBus callback for unmount.
  void HandleUnmountCallback(UnmountCallback callback,
                             dbus::Response* response) {
    if (!response) {
      DLOG(ERROR) << "Unmount: failed to call smbprovider";
      std::move(callback).Run(-EIO);
    }
    dbus::MessageReader reader(response);
    std::move(callback).Run(GetErrorFromReader(&reader));
  }

  dbus::ObjectProxy* proxy_ = nullptr;
  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<SmbProviderClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SmbProviderClientImpl);
};

}  // namespace

SmbProviderClient::SmbProviderClient() {}

SmbProviderClient::~SmbProviderClient() {}

// static
SmbProviderClient* SmbProviderClient::Create() {
  return new SmbProviderClientImpl();
}

}  // namespace chromeos
