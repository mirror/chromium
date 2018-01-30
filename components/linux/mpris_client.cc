// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/linux/mpris_client.h"

#include "components/dbus/dbus_thread_linux.h"
#include "dbus/bus.h"
#include "dbus/exported_object.h"
#include "dbus/object_manager.h"

namespace {

const char kBusPath[] = "/org/mpris/MediaPlayer2";

std::string BusName() {
  return "org.mpris.MediaPlayer2.chromium.instance" +
  std::to_string(getpid());
}

void IgnorePropertyChanged(const std::string& property_name) {}

void QuitMethodCallback(dbus::MethodCall* method_call,
                        dbus::ExportedObject::ResponseSender response_sender) {
  printf("quit called\n");
  response_sender.Run(dbus::Response::FromMethodCall(method_call));
}

void OnExported(const std::string& interface_name,
                const std::string& method_name,
                bool success) {
  if (success) {
    printf("exported %s %s\n", interface_name.c_str(), method_name.c_str());
  }
  LOG_IF(WARNING, !success)
      << "Failed to export " << interface_name << "." << method_name;
}

}  // namespace

class Mp2Interface : public dbus::ObjectManager::Interface {
 public:
  Mp2Interface() {}
  ~Mp2Interface() override {}

  dbus::PropertySet* CreateProperties(
      dbus::ObjectProxy* object_proxy,
      const dbus::ObjectPath& object_path,
      const std::string& interface_name) override {
    Properties* properties = new Properties(object_proxy, interface_name);
    return static_cast<dbus::PropertySet*>(properties);
  }

 private:
  struct Properties : public dbus::PropertySet {
    dbus::Property<bool> can_quit;

    Properties(dbus::ObjectProxy* object_proxy,
               const std::string& interface_name)
        : PropertySet(object_proxy,
                      interface_name,
                      base::Bind(&IgnorePropertyChanged)) {
      RegisterProperty("CanQuit", &can_quit);
    }
  };

  DISALLOW_COPY_AND_ASSIGN(Mp2Interface);
};

MprisClient::MprisClient() : weak_ptr_factory_(this) {
  dbus::Bus::Options bus_options;
  bus_options.bus_type = dbus::Bus::SESSION;
  bus_options.connection_type = dbus::Bus::PRIVATE;
  bus_options.dbus_task_runner = dbus_thread_linux::GetDBusTaskRunner();
  bus_ = base::MakeRefCounted<dbus::Bus>(bus_options);

  bus_->RequestOwnership(
      BusName(), dbus::Bus::REQUIRE_PRIMARY,
      base::Bind(&MprisClient::OnOwnership, weak_ptr_factory_.GetWeakPtr()));
}

MprisClient::~MprisClient() {
  bus_->ShutdownAndBlock();
}

void MprisClient::OnOwnership(const std::string& service_name, bool success) {
  if (success) {
    printf("service name is %s\n", service_name.c_str());

    // mp2_manager_ =
    //     bus_->GetObjectManager(service_name, dbus::ObjectPath(kBusPath));
    // mp2_manager_->RegisterInterface("org.mpris.MediaPlayer2",
    //                                 new Mp2Interface());

    exported_object_ = bus_->GetExportedObject(dbus::ObjectPath(kBusPath));
    exported_object_->ExportMethod("org.Mpris.MediaPlayer2", "Quit",
                                   base::Bind(&QuitMethodCallback),
                                   base::Bind(&OnExported));
  }
}
