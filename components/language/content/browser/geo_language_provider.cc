// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/browser/geo_language_provider.h"

#include "base/memory/singleton.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

// Binds the specified ConnectorRequest using the UI thread Connector.
// Must be called on UI thread.
// Used to bind a ConnectorRequest from a non-UI thread.
void BindConnectorRequest(service_manager::mojom::ConnectorRequest request) {
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(request));
}

const char kBcp47Undetermined[] = "und";

}  // namespace

GeoLanguageProvider::GeoLanguageProvider()
    : language_(kBcp47Undetermined),
      background_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      weak_ptr_factory_(this) {
  // Constructor is not required to run on background_task_runner_:
  DETACH_FROM_SEQUENCE(background_sequence_checker_);
}

GeoLanguageProvider::~GeoLanguageProvider() {}

/* static */
GeoLanguageProvider* GeoLanguageProvider::GetInstance() {
  return base::Singleton<GeoLanguageProvider>::get();
}

void GeoLanguageProvider::StartUp() {
  background_task_runner_->PostTask(
      FROM_HERE, base::Bind(&GeoLanguageProvider::BackgroundStartUp,
                            weak_ptr_factory_.GetWeakPtr()));
}

std::string GeoLanguageProvider::CurrentGeoLanguage() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return language_;
}

void GeoLanguageProvider::BackgroundStartUp() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(background_sequence_checker_);

  // Get a Service Manager Connector on our sequence.
  service_manager::mojom::ConnectorRequest connector_request;
  std::unique_ptr<service_manager::Connector> connector =
      service_manager::Connector::Create(&connector_request);
  // Use the UI thread Connector to bind our Connector.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&BindConnectorRequest, base::Passed(&connector_request)));

  // Use our Connector to bind ip_geolocation_service_.
  DCHECK(!ip_geolocation_service_.is_bound());
  connector->BindInterface("device" /* TODO(amoylan): Define constant */,
                           &ip_geolocation_service_);
  // TODO(amoylan): Set error handler.
}

void GeoLanguageProvider::SetGeoLanguage(const std::string& language) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  language_ = language;
}
