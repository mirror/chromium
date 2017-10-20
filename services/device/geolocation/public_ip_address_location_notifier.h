// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_LOCATION_NOTIFIER_H_
#define SERVICES_DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_LOCATION_NOTIFIER_H_

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "device/geolocation/geolocation_provider.h"
#include "device/geolocation/geoposition.h"
#include "device/geolocation/network_location_request.h"
#include "net/base/network_change_notifier.h"

namespace device {

class NetworkLocationRequest;
struct WifiData;

// Provides subscribers with updates of the device's approximate geographic
// location inferred from its publicly-visible IP address.
// Sequencing:
// * Must be created, used, and destroyed on the same sequence.
class PublicIpAddressLocationNotifier
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  // Creates a notifier that uses the specified Google |api_key| and a URL
  // request context produced by |request_context_producer| for network location
  // requests.
  PublicIpAddressLocationNotifier(
      GeolocationProvider::RequestContextProducer request_context_producer,
      const std::string& api_key);
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

  // Gets a WeakPtr to this object, to aid users of AddLocationUpdateCallback
  // who might outlive this object (e.g., mojo::StrongBindings).
  base::WeakPtr<PublicIpAddressLocationNotifier> GetWeakPtr();

 private:
  // Sequence checker for tasks that must run on the same sequence as
  // the |task_runner| passed to the constructor.
  SEQUENCE_CHECKER(sequence_checker_);

  // Response callback for request_context_producer_. Initializes
  // network_location_request_.
  void OnRequestContextResponse(
      scoped_refptr<net::URLRequestContextGetter> context_getter);

  // NetworkChangeNotifier::NetworkChangeObserver:
  // Network change notifications tend to come in a cluster in a short time, so
  // this just sets a task to run ReactToNetworkChange after a short time.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // Actually react to a network change, starting a network geolocation request
  // if any clients are waiting.
  void ReactToNetworkChange();

  // TODO(amoylan): Add MaybeMakeLocationReqest.
  //                This should send a request only if there are any waiting
  //                subscribers.

  // Begins a network location request, by first obtaining a
  // URLRequestContextGetter, then continuing in
  // MakeNetworkLocationRequestWithContext.
  void MakeNetworkLocationRequest();

  // Creates network_location_request_ and starts the network request, which
  // will invoke OnNetworkLocationResponse when done.
  void MakeNetworkLocationRequestWithContext(
      scoped_refptr<net::URLRequestContextGetter> context_getter);

  // TODO(amoylan): Cached geoposition for new subscribers.
  // TODO(amoylan): Last network change time.

  // Completion callback for network_location_request_.
  void OnNetworkLocationResponse(const Geoposition& position,
                                 bool server_error,
                                 const WifiData& wifi_data);

  // Google API key for network geolocation requests.
  const std::string api_key_;

  // Callback to produce a URL request context for network geolocation requests.
  const GeolocationProvider::RequestContextProducer request_context_producer_;

  // Cancelable closure to absorb overlapping delayed calls to
  // ReactToNetworkChange.
  base::CancelableClosure react_to_network_change_closure_;

  // Used to make calls to the Maps geolocate API.
  std::unique_ptr<NetworkLocationRequest> network_location_request_;

  base::CallbackList<void(const Geoposition&)> callbacks_;

  // Weak references to |this| for posted tasks.
  base::WeakPtrFactory<PublicIpAddressLocationNotifier> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PublicIpAddressLocationNotifier);
};

}  // namespace device

#endif  // SERVICES_DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_LOCATION_NOTIFIER_H_
