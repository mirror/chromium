// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/browser/geo_language_provider.h"

#include "base/memory/singleton.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/device/public/interfaces/public_ip_address_geolocation_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

// Don't start requesting updates to IP-based approximation geolocation until
// this long after receiving the last one.
constexpr base::TimeDelta kMinUpdatePeriod = base::TimeDelta::FromDays(1);

}  // namespace

GeoLanguageProvider::GeoLanguageProvider()
    : languages_(),
      creation_task_runner_(base::SequencedTaskRunnerHandle::Get()),
      background_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {
  // Constructor is not required to run on background_task_runner_:
  DETACH_FROM_SEQUENCE(background_sequence_checker_);
}

GeoLanguageProvider::~GeoLanguageProvider() {}

/* static */
GeoLanguageProvider* GeoLanguageProvider::GetInstance() {
  return base::Singleton<GeoLanguageProvider, base::LeakySingletonTraits<
                                                  GeoLanguageProvider>>::get();
}

void GeoLanguageProvider::StartUp(
    service_manager::Connector* const service_manager_connector) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(creation_sequence_checker_);
  // Clone service_manager_connector for use on background_task_runner_.
  service_manager_connector_ = service_manager_connector->Clone();

  // Continue startup in the background.
  background_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&GeoLanguageProvider::BackgroundStartUp,
                                base::Unretained(this)));
}

std::vector<std::string> GeoLanguageProvider::CurrentGeoLanguages() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(creation_sequence_checker_);
  return languages_;
}

void GeoLanguageProvider::BackgroundStartUp() {
  // This binds background_sequence_checker_.
  DCHECK_CALLED_ON_VALID_SEQUENCE(background_sequence_checker_);

  // Initialize location->language lookup library.
  language_code_locator_ = std::make_unique<language::LanguageCodeLocator>();

  // Make initial query.
  QueryNextPosition();
}

void GeoLanguageProvider::BindIpGeolocationService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(background_sequence_checker_);
  DCHECK(!ip_geolocation_service_.is_bound());

  // Bind a PublicIpAddressGeolocationProvider.
  device::mojom::PublicIpAddressGeolocationProviderPtr ip_geolocation_provider;
  service_manager_connector_->BindInterface(
      device::mojom::kServiceName, mojo::MakeRequest(&ip_geolocation_provider));

  // Use the PublicIpAddressGeolocationProvider to bind ip_geolocation_service_.
  ip_geolocation_provider->CreateGeolocation(
      mojo::MakeRequest(&ip_geolocation_service_));
  // No error handler required: If the connection is broken, QueryNextPosition
  // will bind it again.
}

void GeoLanguageProvider::QueryNextPosition() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(background_sequence_checker_);

  if (!ip_geolocation_service_.is_bound()) {
    BindIpGeolocationService();
  }

  ip_geolocation_service_->QueryNextPosition(base::BindOnce(
      &GeoLanguageProvider::OnIpGeolocationResponse, base::Unretained(this)));
}

void GeoLanguageProvider::OnIpGeolocationResponse(
    device::mojom::GeopositionPtr geoposition) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(background_sequence_checker_);

  const std::vector<std::string> languages =
      language_code_locator_->GetLanguageCode(geoposition->latitude,
                                              geoposition->longitude);

  // Update current languages on UI thread.
  creation_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&GeoLanguageProvider::SetGeoLanguages,
                                base::Unretained(this), languages));

  // Post a task to request a fresh lookup after |kMinUpdatePeriod|.
  background_task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&GeoLanguageProvider::QueryNextPosition,
                     base::Unretained(this)),
      kMinUpdatePeriod);
}

void GeoLanguageProvider::SetGeoLanguages(
    const std::vector<std::string>& languages) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(creation_sequence_checker_);
  languages_ = languages;
}
