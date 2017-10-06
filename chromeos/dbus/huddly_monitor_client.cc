// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/huddly_monitor_client.h"

#include <stdint.h>

#include <algorithm>
#include <memory>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/threading/platform_thread.h"
#include "base/timer/timer.h"
#include "chromeos/chromeos_switches.h"
#include "components/device_event_log/device_event_log.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"

namespace chromeos {

const char kHuddlyMonitorServiceName[] = "org.chromium.huddlymonitor";
const char kHuddlyMonitorServicePath[] = "/org/chromium/huddlymonitor";
const char kHuddlyMonitorInterface[] = "org.chromium.huddlymonitor";
const char kUpdateConditionMethod[] = "UpdateCondition";

class HuddlyMonitorClientImpl : public HuddlyMonitorClient {
 public:
  HuddlyMonitorClientImpl()
      : huddly_monitor_proxy_(NULL),
        weak_ptr_factory_(this) {}

  ~HuddlyMonitorClientImpl() override { }

  void UpdateCondition(bool updated_condition, uint64_t timestamp) override {
    dbus::MethodCall method_call(
        kHuddlyMonitorInterface,
        kUpdateConditionMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendBool(updated_condition);
    writer.AppendInt64(timestamp);
    huddly_monitor_proxy_->CallMethod(
        &method_call,
        dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        dbus::ObjectProxy::EmptyResponseCallback());
  }

 protected:
  void Init(dbus::Bus* bus) override {
    huddly_monitor_proxy_ = bus->GetObjectProxy(
        kHuddlyMonitorServiceName,
        dbus::ObjectPath(kHuddlyMonitorServicePath));

  }

 private:
  dbus::ObjectProxy* huddly_monitor_proxy_;
  base::WeakPtrFactory<HuddlyMonitorClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HuddlyMonitorClientImpl);
};

HuddlyMonitorClient::HuddlyMonitorClient() {
}

HuddlyMonitorClient::~HuddlyMonitorClient() {
}

// static
HuddlyMonitorClient* HuddlyMonitorClient::Create() {
  return new HuddlyMonitorClientImpl();
}

}  // namespace chromeos
