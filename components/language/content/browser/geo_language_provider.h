// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANGUAGE_CONTENT_BROWSER_GEO_LANGUAGE_PROVIDER_H_
#define COMPONENTS_LANGUAGE_CONTENT_BROWSER_GEO_LANGUAGE_PROVIDER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "services/device/public/interfaces/public_ip_address_geolocator.mojom.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

// GeoLanguageProvider is responsible for providing a "local" language derived
// from the approximate geolocation of the device based only on its public IP
// address.
// Singleton class. Access through GetInstance().
// The main client method CurrentGeoLanguage() must be called on the UI thread.
class GeoLanguageProvider {
 public:
  static GeoLanguageProvider* GetInstance();

  // Call this once near browser startup. Begins ongoing geo-language updates.
  // Actual start-up happens in a low-priority background task.
  // * Subscribes to IP-geolocation service.
  // * Initializes location->language mapping.
  // Until the first IP geolocation completes, CurrentGeoLanguage() will
  // return "und".
  void StartUp();

  // Returns the inferred local language based on the most recently obtained
  // approximate public-IP geolocation of the device.
  // * Must be called on UI thread.
  // * Returns a BCP-47 language code.
  // * May return "und" (BCP-47 "Undetermined"), for example in these cases:
  //   - StartUp() not yet called
  //   - Geolocation failed
  //   - Geolocation pending
  //   - Geolocation succeeded but no local language is mapped to that location
  std::string CurrentGeoLanguage() const;

 private:
  GeoLanguageProvider();
  ~GeoLanguageProvider();
  friend struct base::DefaultSingletonTraits<GeoLanguageProvider>;

  // Performs actual work described in StartUp() above.
  void BackgroundStartUp();

  // TODO(amoylan): Reponse callback from IP geolocation service.
  //                This should invoke Renjie's code and then set a task to
  //                request a new position in 24 hrs.

  // Updates the BCP-47 language code that will be returned by calls to
  // CurrentGeoLanguage().
  // Must be called on the UI thread.
  void SetGeoLanguage(const std::string& language);

  // BCP-47 language code inferred from public-IP geolocation.
  // May be "und" (undetermined).
  std::string language_;

  device::mojom::PublicIpAddressGeolocatorPtr ip_geolocation_service_;

  // Runner for tasks that should run in the background (not in the UI thread).
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Sequence checker for background_task_runner_.
  SEQUENCE_CHECKER(background_sequence_checker_);

  // Weak references to |this| for background_task_runner_ tasks.
  base::WeakPtrFactory<GeoLanguageProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GeoLanguageProvider);
};

#endif  // COMPONENTS_LANGUAGE_CONTENT_BROWSER_GEO_LANGUAGE_PROVIDER_H_
