// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/battery/BatteryDispatcher.h"

#include "platform/mojo/MojoHelper.h"
#include "platform/wtf/Assertions.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

// static
BatteryDispatcher* BatteryDispatcher::From(LocalFrame& frame) {
  auto* dispatcher = static_cast<BatteryDispatcher*>(
      Supplement<LocalFrame>::From(frame, SupplementName()));
  if (!dispatcher) {
    dispatcher = new BatteryDispatcher(frame);
    Supplement<LocalFrame>::ProvideTo(frame, SupplementName(), dispatcher);
  }
  return dispatcher;
}

BatteryDispatcher::BatteryDispatcher(LocalFrame& frame)
    : Supplement<LocalFrame>(frame), has_latest_data_(false) {}

DEFINE_TRACE(BatteryDispatcher) {
  Supplement<LocalFrame>::Trace(visitor);
  PlatformEventDispatcher::Trace(visitor);
}

// static
const char* BatteryDispatcher::SupplementName() {
  return "BatteryDispatcher";
}

void BatteryDispatcher::QueryNextStatus() {
  monitor_->QueryNextStatus(ConvertToBaseCallback(
      WTF::Bind(&BatteryDispatcher::OnDidChange, WrapWeakPersistent(this))));
}

void BatteryDispatcher::OnDidChange(
    device::mojom::blink::BatteryStatusPtr battery_status) {
  QueryNextStatus();

  DCHECK(battery_status);

  UpdateBatteryStatus(
      BatteryStatus(battery_status->charging, battery_status->charging_time,
                    battery_status->discharging_time, battery_status->level));
}

void BatteryDispatcher::UpdateBatteryStatus(
    const BatteryStatus& battery_status) {
  battery_status_ = battery_status;
  has_latest_data_ = true;
  NotifyControllers();
}

void BatteryDispatcher::StartListening() {
  DCHECK(!monitor_.is_bound());
  GetSupplementable()->GetInterfaceProvider().GetInterface(
      mojo::MakeRequest(&monitor_));
  QueryNextStatus();
}

void BatteryDispatcher::StopListening() {
  monitor_.reset();
  has_latest_data_ = false;
}

}  // namespace blink
