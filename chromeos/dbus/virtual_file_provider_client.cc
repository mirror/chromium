// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/virtual_file_provider_client.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

namespace chromeos {

namespace {

// TODO(hashimoto): Share these constants with the service.
// D-Bus service constants.
constexpr char kVirtualFileProviderInterface[] =
    "org.chromium.VirtualFileProviderInterface";
constexpr char kVirtualFileProviderServicePath[] =
    "/org/chromium/VirtualFileProvider";
constexpr char kVirtualFileProviderServiceName[] =
    "org.chromium.VirtualFileProvider";

// Method names.
constexpr char kOpenFileMethod[] = "OpenFile";

// Signal names.
constexpr char kReadRequestSignal[] = "ReadRequest";
constexpr char kReleaseIDSignal[] = "ReleaseID";

class VirtualFileProviderClientImpl : public VirtualFileProviderClient {
 public:
  VirtualFileProviderClientImpl() : weak_ptr_factory_(this) {}
  ~VirtualFileProviderClientImpl() override = default;

  // VirtualFileProviderClient override:
  void OpenFile(int64_t size, OpenFileCallback callback) override {
    dbus::MethodCall method_call(kVirtualFileProviderInterface,
                                 kOpenFileMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendInt64(size);
    proxy_->CallMethod(&method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                       base::Bind(&VirtualFileProviderClientImpl::OnOpenFile,
                                  weak_ptr_factory_.GetWeakPtr(),
                                  base::Passed(std::move(callback))));
  }

  void SetReadRequestHandler(const ReadRequestHandler& handler) override {
    read_request_handler_ = handler;
  }

  void SetReleaseIDHandler(const ReleaseIDHandler& handler) override {
    release_id_handler_ = handler;
  }

 protected:
  // DBusClient override.
  void Init(dbus::Bus* bus) override {
    proxy_ =
        bus->GetObjectProxy(kVirtualFileProviderServiceName,
                            dbus::ObjectPath(kVirtualFileProviderServicePath));
    proxy_->ConnectToSignal(
        kVirtualFileProviderInterface, kReadRequestSignal,
        base::Bind(&VirtualFileProviderClientImpl::OnReadRequest,
                   weak_ptr_factory_.GetWeakPtr()),
        base::Bind(&VirtualFileProviderClientImpl::OnSignalConnected,
                   weak_ptr_factory_.GetWeakPtr()));
    proxy_->ConnectToSignal(
        kVirtualFileProviderInterface, kReleaseIDSignal,
        base::Bind(&VirtualFileProviderClientImpl::OnReleaseID,
                   weak_ptr_factory_.GetWeakPtr()),
        base::Bind(&VirtualFileProviderClientImpl::OnSignalConnected,
                   weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  // Handles signal connection result.
  void OnSignalConnected(const std::string& interface,
                         const std::string& signal,
                         bool succeeded) {
    LOG_IF(ERROR, !succeeded)
        << "Signal connection failed: " << interface << "." << signal;
  }

  // Handles ReadRequest signal.
  void OnReadRequest(dbus::Signal* signal) {
    dbus::MessageReader reader(signal);
    std::string id;
    int64_t offset = 0;
    int64_t size = 0;
    base::ScopedFD pipe_write_end;
    if (!reader.PopString(&id) || !reader.PopInt64(&offset) ||
        !reader.PopInt64(&size) || !reader.PopFileDescriptor(&pipe_write_end)) {
      LOG(ERROR) << "Invalid signal: " << signal->ToString();
      return;
    }
    if (!read_request_handler_.is_null())
      read_request_handler_.Run(id, offset, size, std::move(pipe_write_end));
  }

  // Handles ReleaseID signal.
  void OnReleaseID(dbus::Signal* signal) {
    dbus::MessageReader reader(signal);
    std::string id;
    if (!reader.PopString(&id)) {
      LOG(ERROR) << "Invalid signal: " << signal->ToString();
      return;
    }
    if (!release_id_handler_.is_null())
      release_id_handler_.Run(id);
  }

  // Runs the callback with OpenFile method call result.
  void OnOpenFile(OpenFileCallback callback, dbus::Response* response) {
    if (!response) {
      std::move(callback).Run(std::string(), base::ScopedFD());
      return;
    }
    dbus::MessageReader reader(response);
    std::string id;
    base::ScopedFD fd;
    if (!reader.PopString(&id) || !reader.PopFileDescriptor(&fd)) {
      LOG(ERROR) << "Invalid method call result.";
      std::move(callback).Run(std::string(), base::ScopedFD());
      return;
    }
    std::move(callback).Run(id, std::move(fd));
  }

  dbus::ObjectProxy* proxy_ = nullptr;

  ReadRequestHandler read_request_handler_;
  ReleaseIDHandler release_id_handler_;

  base::WeakPtrFactory<VirtualFileProviderClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VirtualFileProviderClientImpl);
};

}  // namespace

VirtualFileProviderClient::VirtualFileProviderClient() = default;

VirtualFileProviderClient::~VirtualFileProviderClient() = default;

// static
VirtualFileProviderClient* VirtualFileProviderClient::Create() {
  return new VirtualFileProviderClientImpl();
}

}  // namespace chromeos
