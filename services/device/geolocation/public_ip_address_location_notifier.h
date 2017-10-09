// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_LOCATION_NOTIFIER_H_
#define SERVICES_DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_LOCATION_NOTIFIER_H_

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/macros.h"
#include "device/geolocation/geoposition.h"
#include "net/base/network_change_notifier.h"

namespace device {

// Provides subscribers with updates of the device's approximate geographic
// location inferred from its publicly-visible IP address.
// * Must be used and destructed on the same sequence as the |task_runner| with
//   which is is constructed.
class PublicIpAddressLocationNotifier
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  // Creates a notifier that will run on the specified |task_runner|.
  // TODO(amoylan): Inject URL request context & api key.
  PublicIpAddressLocationNotifier(base::SequencedTaskRunner* task_runner);
  ~PublicIpAddressLocationNotifier() override;

  using LocationUpdateCallback =
      base::RepeatingCallback<void(const Geoposition&)>;
  using LocationUpdateSubscription = std::unique_ptr<
      base::CallbackList<void(const Geoposition&)>::Subscription>;

  // TODO(amoylan): Change the following to QueryNextLocationAfterTime(...).
  //                Subscribers are automatically removed upon response.
  //                They should specify a timestamp of their latest known
  //                location, so this class can decide whether they need an
  //                update or not.
  //                New subscribers get the cached result immediately.
  // Adds to the list of callbacks that will be notified of changes in the
  // device's inferred location.
  // * Must be called on the same sequence as the |task_runner| with which this
  //   object was constructed.
  // * |callback| will be invoked on the same sequence.
  // * |callback| is invoked immediately if a location has already been
  //   inferred.
  // * |callback| will be removed from the list when the returned
  //   LocationUpdateSubscription is destroyed.
  LocationUpdateSubscription AddLocationUpdateCallback(
      LocationUpdateCallback callback);

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  // Actual startup, which runs on the |task_runner| passed to the constructor.
  void Start();

  // NetworkChangeNotifier::NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // TODO(amoylan): Add MaybeMakeLocationReqest.
  //                This should send a request only if there are any waiting
  //                subscribers.

  // TODO(amoylan): Cached geoposition for new subscribers.

  // TODO(amoylan): NetworkLocationRequest
  // TODO(amoylan): LocationResponseCallback for NetworkLocationRequest

  base::CallbackList<void(const Geoposition&)> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(PublicIpAddressLocationNotifier);
};

}  // namespace device

#endif  // SERVICES_DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_LOCATION_NOTIFIER_H_
