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
#include "components/language/language_code_locator.h"
#include "device/geolocation/public/interfaces/geolocation.mojom.h"

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
  // Until the first IP geolocation completes, CurrentGeoLanguages() will return
  // an empty list.
  void StartUp();

  // Returns the inferred ranked list of local languages based on the most
  // recently obtained approximate public-IP geolocation of the device.
  // * Must be called on UI thread.
  // * Returns a list of  BCP-47 language codes.
  // * Returns an empty list in these cases:
  //   - StartUp() not yet called
  //   - Geolocation failed
  //   - Geolocation pending
  //   - Geolocation succeeded but no local language is mapped to that location
  std::vector<std::string> CurrentGeoLanguages() const;

 private:
  GeoLanguageProvider();
  ~GeoLanguageProvider();
  friend struct base::DefaultSingletonTraits<GeoLanguageProvider>;

  // Performs actual work described in StartUp() above.
  void BackgroundStartUp();

  // Binds |ip_geolocation_service_| using a service_manager::Connector.
  void BindIpGeolocationService();

  // Requests the next available IP-based approximate geolocation from
  // |ip_geolocation_service_|, binding |ip_geolocation_service_| first if
  // necessary.
  void QueryNextPosition();

  // Updates the list of BCP-47 language codes that will be returned by calls to
  // CurrentGeoLanguages().
  // Must be called on the UI thread.
  void SetGeoLanguages(const std::vector<std::string>& languages);

  // Callback for updates from |ip_geolocation_service_|.
  void OnIpGeolocationResponse(device::mojom::GeopositionPtr geoposition);

  // List of BCP-47 language code inferred from public-IP geolocation.
  // May be empty. See comment on CurrentGeoLanguages() above.
  std::vector<std::string> languages_;

  // Connection to the IP geolocation service.
  device::mojom::GeolocationPtr ip_geolocation_service_;

  // Location -> Language lookup library.
  std::unique_ptr<language::LanguageCodeLocator> language_code_locator_;

  // Runner for tasks that should run in the background (not in the UI thread).
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Sequence checker for background_task_runner_.
  SEQUENCE_CHECKER(background_sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(GeoLanguageProvider);
};

#endif  // COMPONENTS_LANGUAGE_CONTENT_BROWSER_GEO_LANGUAGE_PROVIDER_H_
