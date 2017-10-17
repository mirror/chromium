// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/smb_client_client.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

namespace smbclient {
enum SMBErrorType {
  SMB_ERROR_NONE = 0,
  SMB_ERROR_UNKNOWN = 1,
  SMB_ERROR_DBUS_FAIL = 2,
};
}

namespace chromeos {

namespace {

class SmbClientClientImpl : public SmbClientClient {
 public:
  SmbClientClientImpl() : weak_ptr_factory_(this) {}

  ~SmbClientClientImpl() override {}

  void CommunicateToService(const std::string& directoryPath,
                            EntryDataMethodCallback callback) override {
    dbus::MethodCall method_call(smbclient::kSmbClientInterface,
                                 smbclient::kReadDirectory);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(directoryPath);
    proxy_->CallMethod(&method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                       base::Bind(&SmbClientClientImpl::HandleJoinCallback,
                                  weak_ptr_factory_.GetWeakPtr(),
                                  base::Passed(std::move(callback))));
  }

 protected:
  // DBusClient override.
  void Init(dbus::Bus* bus) override {
    proxy_ =
        bus->GetObjectProxy(smbclient::kSmbClientServiceName,
                            dbus::ObjectPath(smbclient::kSmbClientServicePath));
  }

 private:
  // Runs the callback with the method call result.
  void HandleJoinCallback(EntryDataMethodCallback callback,
                          dbus::Response* response) {
    std::vector<EntryData> results;
    if (!response) {
      LOG(ERROR) << "Join couldn't call SMB Client";
      EntryData result;
      callback.Run(DBUS_METHOD_CALL_FAILURE, results);
      return;
    }
    //    dbus::MessageReader reader(response);
    //    std::vector<std::string> result;
    //    if (!reader.PopArrayOfStrings(&result)) {
    //      LOG(ERROR) << "Invalid response: " << response->ToString();
    //      callback.Run(DBUS_METHOD_CALL_FAILURE, result);
    //      return;
    //    }

    dbus::MessageReader reader(response);
    int32_t error;
    if (!reader.PopInt32(&error)) {
      LOG(ERROR) << "Invalid response for error code: " << error;
      callback.Run(DBUS_METHOD_CALL_FAILURE, results);
      return;
    }
    LOG(ERROR) << "Error?: " << error;
    dbus::MessageReader array_reader(NULL);
    if (!reader.PopArray(&array_reader)) {
      LOG(ERROR) << "Invalid response: " << response->ToString();
      callback.Run(DBUS_METHOD_CALL_FAILURE, results);
      return;
    }
    LOG(WARNING) << "HANDLING CALLLLL BACKKKK";

    while (array_reader.HasMoreData()) {
      EntryData result;
      dbus::MessageReader struct_reader(nullptr);
      if (!array_reader.PopStruct(&struct_reader)) {
        LOG(ERROR) << "Invalid response: " << response->ToString();
        callback.Run(DBUS_METHOD_CALL_FAILURE, results);
        return;
      }
      if (!struct_reader.PopBool(&result.is_directory)) {
        LOG(ERROR) << "cant find is directory: " << response->ToString();
        callback.Run(DBUS_METHOD_CALL_FAILURE, results);
        return;
      }
      if (!struct_reader.PopString(&result.name)) {
        LOG(ERROR) << "cant find name: " << response->ToString();
        callback.Run(DBUS_METHOD_CALL_FAILURE, results);
        return;
      }
      if (!struct_reader.PopString(&result.fullPath)) {
        LOG(ERROR) << "cant find fullpath: " << response->ToString();
        callback.Run(DBUS_METHOD_CALL_FAILURE, results);
        return;
      }
      if (!struct_reader.PopInt64(&result.size)) {
        LOG(ERROR) << "cant find size: " << response->ToString();
        callback.Run(DBUS_METHOD_CALL_FAILURE, results);
        return;
      }
      if (!struct_reader.PopInt64(&result.modification_time)) {
        LOG(ERROR) << "cant find modification time: " << response->ToString();
        callback.Run(DBUS_METHOD_CALL_FAILURE, results);
        return;
      }
      results.push_back(result);
    }

    LOG(WARNING) << "Successfully handled join callback";
    std::move(callback).Run(DBUS_METHOD_CALL_SUCCESS, results);
  }

  dbus::ObjectProxy* proxy_ = nullptr;
  base::WeakPtrFactory<SmbClientClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SmbClientClientImpl);
};

}  // namespace

SmbClientClient::SmbClientClient() {}

SmbClientClient::~SmbClientClient() {}

// static
SmbClientClient* SmbClientClient::Create() {
  return new SmbClientClientImpl();
}

}  // namespace chromeos
