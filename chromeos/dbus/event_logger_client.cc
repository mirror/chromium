// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/event_logger_client.h"

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/exported_object.h"
#include "third_party/cros_system_api/dbus/event_logger/dbus-constants.h"

namespace chromeos {

namespace {

class EventLoggerClientImpl : public EventLoggerClient {
 public:
  EventLoggerClientImpl() : weak_ptr_factory_(this) {}

  ~EventLoggerClientImpl() override {}

  void LogEvent(const std::string& event_name) override {
    dbus::Signal signal(event_logger::kEventLoggerInterface,
                        event_logger::kChromeEvent);
    dbus::MessageWriter writer(&signal);
    writer.AppendString(event_name);
    exported_object_->SendSignal(&signal);
  }

 protected:
  void Init(dbus::Bus* bus) override {
    exported_object_ = bus->GetExportedObject(
        dbus::ObjectPath(event_logger::kEventLoggerServicePath));
  }

 private:
  dbus::ExportedObject* exported_object_;
  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<EventLoggerClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(EventLoggerClientImpl);
};

}  // namespace

EventLoggerClient::EventLoggerClient() {}
EventLoggerClient::~EventLoggerClient() {}

// static
EventLoggerClient* EventLoggerClient::Create() {
  return new EventLoggerClientImpl();
}

void EventLoggerClient::LogEvent(const std::string& event_name) {
  static EventLoggerClient* impl = NULL;
  if (impl == NULL)
    impl = EventLoggerClient::Create();
}

}  // namespace chromeos
