// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/browser/geo_language_provider.h"

#include "base/memory/singleton.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "services/device/public/interfaces/public_ip_address_geolocation_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

// Don't start requesting updates to IP-based approximation geolocation until
// this long after receiving the last one.
constexpr base::TimeDelta kMinUpdatePeriod = base::TimeDelta::FromDays(1);

// Binds the specified ConnectorRequest using the UI thread Connector.
// Must be called on UI thread.
// Used to bind a ConnectorRequest from a non-UI thread.
void BindConnectorRequest(service_manager::mojom::ConnectorRequest request) {
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(request));
}

}  // namespace

GeoLanguageProvider::GeoLanguageProvider()
    : languages_(),
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

void GeoLanguageProvider::StartUp() {
  background_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&GeoLanguageProvider::BackgroundStartUp,
                                base::Unretained(this)));
}

std::vector<std::string> GeoLanguageProvider::CurrentGeoLanguages() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return languages_;
}

void GeoLanguageProvider::BackgroundStartUp() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(background_sequence_checker_);

  // Initialize location->language lookup library.
  language_code_locator_ = std::make_unique<language::LanguageCodeLocator>();

  // Make initial query.
  QueryNextPosition();
}

void GeoLanguageProvider::BindIpGeolocationService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(background_sequence_checker_);
  DCHECK(!ip_geolocation_service_.is_bound());

  // Get a Service Manager Connector on our sequence.
  service_manager::mojom::ConnectorRequest connector_request;
  std::unique_ptr<service_manager::Connector> connector =
      service_manager::Connector::Create(&connector_request);
  // Use the UI thread Connector to bind our Connector.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&BindConnectorRequest, base::Passed(&connector_request)));

  // Use our Connector to bind ip_geolocation_service_.
  device::mojom::PublicIpAddressGeolocationProviderPtr ip_geolocation_provider;
  connector->BindInterface("device" /* TODO(amoylan): Define constant */,
                           mojo::MakeRequest(&ip_geolocation_provider));

  // Use ip_geolocation_provider to bind ip_geolocation_service_.
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
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)
      ->PostTask(FROM_HERE,
                 base::BindOnce(&GeoLanguageProvider::SetGeoLanguages,
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
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  languages_ = languages;
}
