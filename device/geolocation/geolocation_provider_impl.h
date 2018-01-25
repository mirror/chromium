// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_GEOLOCATION_PROVIDER_IMPL_H_
#define DEVICE_GEOLOCATION_GEOLOCATION_PROVIDER_IMPL_H_

#include <list>
#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/threading/thread.h"
#include "device/geolocation/geolocation_export.h"
#include "device/geolocation/geolocation_provider.h"
#include "device/geolocation/public/cpp/location_provider.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/interfaces/geolocation_control.mojom.h"
#include "services/device/public/interfaces/geoposition.mojom.h"

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
class SingleThreadTaskRunner;
}

namespace device {

// Callback that returns the embedder's custom location provider. This callback
// is provided to the Device Service by its embedder.
using CustomLocationProviderCallback =
    base::Callback<std::unique_ptr<LocationProvider>()>;

// This is the main API to the geolocation subsystem. The application will hold
// a single instance of this class and can register multiple clients to be
// notified of location changes:
// * Callbacks are registered by AddLocationUpdateCallback() and will keep
//   receiving updates until the returned subscription object is destructed.
// The application must instantiate the GeolocationProviderImpl on the UI thread
// and must communicate with it on the same thread.
// The underlying location arbitrator will only be enabled whilst there is at
// least one registered observer or pending callback (and only after
// mojom::UserDidOptIntoLocationServices()). The arbitrator and the location
// providers it uses run on a separate Geolocation thread.
class DEVICE_GEOLOCATION_EXPORT GeolocationProviderImpl
    : public GeolocationProvider,
      public mojom::GeolocationControl,
      public base::Thread {
 public:
  // Callback type for a function that asynchronously produces a
  // URLRequestContextGetter.
  using RequestContextProducer = base::RepeatingCallback<void(
      base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>)>;

  // Optional: Provide a callback to produce a request context for network
  // geolocation requests.
  static void SetRequestContextProducer(
      RequestContextProducer request_context_producer);

  // Optional: Provide a Google API key for network geolocation requests.
  // Call before using Init() on the singleton GetInstance().
  static void SetApiKey(const std::string& api_key);

  // Gets a pointer to the singleton instance of the location relayer, which
  // is in turn bound to the browser's global context objects. This must only be
  // called on the UI thread so that the GeolocationProviderImpl is always
  // instantiated on the same thread. Ownership is NOT returned.
  static GeolocationProviderImpl* GetInstance();

  // Optional: provide a callback which can return a custom location provider
  // from embedder. Call before using Init() on the singleton GetInstance(),
  // and call no more than once.
  static void SetCustomLocationProviderCallback(
      const CustomLocationProviderCallback& callback);

  // GeolocationProvider implementation:
  void OverrideLocationForTesting(const mojom::Geoposition& position) override;

  typedef base::RepeatingCallback<void(const mojom::Geoposition&)>
      LocationUpdateCallback;
  typedef base::CallbackList<void(const mojom::Geoposition&)>::Subscription
      Subscription;

  // |enable_high_accuracy| is used as a 'hint' for the provider preferences for
  // this particular observer, however the observer could receive updates for
  // best available locations from any active provider whilst it is registered.
  std::unique_ptr<Subscription> AddLocationUpdateCallback(
      const LocationUpdateCallback& callback,
      bool enable_high_accuracy);

  bool HighAccuracyLocationInUse();

  // Callback from the LocationArbitrator. Public for testing.
  void OnLocationUpdate(const LocationProvider* provider,
                        const mojom::Geoposition& position);

  void BindGeolocationControlRequest(mojom::GeolocationControlRequest request);

  // mojom::GeolocationControl implementation:
  void UserDidOptIntoLocationServices() override;

  bool user_did_opt_into_location_services_for_testing() {
    return user_did_opt_into_location_services_;
  }

  // Safe to call while there are no GeolocationProviderImpl clients
  // registered.
  void SetArbitratorForTesting(std::unique_ptr<LocationProvider> arbitrator);

 private:
  friend struct base::DefaultSingletonTraits<GeolocationProviderImpl>;
  GeolocationProviderImpl();
  ~GeolocationProviderImpl() override;

  bool OnGeolocationThread() const;

  // Start and stop providers as needed when clients are added or removed.
  void OnClientsChanged();

  // Stops the providers when there are no more registered clients. Note that
  // once the Geolocation thread is started, it will stay alive (but sitting
  // idle without any pending messages).
  void StopProviders();

  // Starts the geolocation providers or updates their options (delegates to
  // arbitrator).
  void StartProviders(bool enable_high_accuracy);

  // Updates the providers on the geolocation thread, which must be running.
  void InformProvidersPermissionGranted();

  // Notifies all registered clients that a position update is available.
  void NotifyClients(const mojom::Geoposition& position);

  // Thread
  void Init() override;
  void CleanUp() override;

  base::CallbackList<void(const mojom::Geoposition&)> high_accuracy_callbacks_;
  base::CallbackList<void(const mojom::Geoposition&)> low_accuracy_callbacks_;

  bool user_did_opt_into_location_services_;
  mojom::Geoposition position_;

  // True only in testing, where we want to use a custom position.
  bool ignore_location_updates_;

  // Used to PostTask()s from the geolocation thread to caller thread.
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // Only to be used on the geolocation thread.
  std::unique_ptr<LocationProvider> arbitrator_;

  mojo::Binding<mojom::GeolocationControl> binding_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationProviderImpl);
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_GEOLOCATION_PROVIDER_IMPL_H_
