// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chromeos/dbus/midis_client.h"

#include <sys/socket.h>

#include <map>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "dbus/values_util.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

extern void pr(const char* fmt, ...);

constexpr char kMidisInterface[] = "org.chromium.MidisInterface";
constexpr char kMidisBootstrapMethod[] = "BootstrapMojoConnection";

void pr(const char* fmt, ...) {
  static FILE* logfile = NULL;
  va_list vl;
  if (!logfile)
    logfile = fopen("/tmp/client_log.txt", "a");
  if (!logfile)
    exit(-1);
  va_start(vl, fmt);
  vfprintf(logfile, fmt, vl);
  fflush(logfile);
  va_end(vl);
}

// MidisClient is used to bootstrap a mojo connection with midis.
// The BootstrapMojoConnection callback should be called during browser
// initialization. It is expected that the midis will be started up / taken down
// during browser startup / shutdown respectively.
class MidisClientImpl : public MidisClient {
 public:
  MidisClientImpl() : bus_(NULL), weak_ptr_factory_(this) {}

  ~MidisClientImpl() override {}

  // Calls GetAll method.  |callback| is called after the method call succeeds.
  base::ScopedFD BootstrapMojoConnection(
      const std::string& service_name,
      const dbus::ObjectPath& object_path) override {
    dbus::ObjectProxy* proxy = bus_->GetObjectProxy(service_name, object_path);
    dbus::MethodCall method_call(kMidisInterface, kMidisBootstrapMethod);
    dbus::MessageWriter writer(&method_call);

    // Create a socketpair, for setting up the Mojo channel.
    int sock_fd[2];
    int ret = socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sock_fd);
    if (ret < 0) {
      PLOG(ERROR) << "Failed to create socket-pair";
      return base::ScopedFD();
    }
    base::ScopedFD client_fd(sock_fd[0]);
    base::ScopedFD server_fd(sock_fd[1]);
    writer.AppendFileDescriptor(server_fd.get());
    proxy->CallMethod(&method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                      dbus::ObjectProxy::EmptyResponseCallback());
    return client_fd;
  }

 protected:
  void Init(dbus::Bus* bus) override { bus_ = bus; }

 private:
  dbus::Bus* bus_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<MidisClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MidisClientImpl);
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// MidisClient

MidisClient::MidisClient() {}

MidisClient::~MidisClient() {}

// static
MidisClient* MidisClient::Create() {
  return new MidisClientImpl();
}

}  // namespace chromeos
