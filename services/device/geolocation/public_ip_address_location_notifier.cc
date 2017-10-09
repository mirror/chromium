// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_location_notifier.h"

namespace device {

PublicIpAddressLocationNotifier::PublicIpAddressLocationNotifier(
    base::SequencedTaskRunner* const task_runner) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  task_runner->PostTask(FROM_HERE,
                        base::BindOnce(&PublicIpAddressLocationNotifier::Start,
                                       base::Unretained(this)));
}

void PublicIpAddressLocationNotifier::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

PublicIpAddressLocationNotifier::~PublicIpAddressLocationNotifier() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

PublicIpAddressLocationNotifier::LocationUpdateSubscription
PublicIpAddressLocationNotifier::AddLocationUpdateCallback(
    LocationUpdateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LocationUpdateSubscription subscription = callbacks_.Add(std::move(callback));
  // TODO(amoylan): Immediately call if appropriate.
  return subscription;
}

void PublicIpAddressLocationNotifier::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Ignore destructive signals.
  if (type == net::NetworkChangeNotifier::CONNECTION_NONE)
    return;
  // TODO(amoylan): Start cancelable task to perform an update in one minute.
}

}  // namespace device
