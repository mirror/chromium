// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/resource_coordinator_web_contents_observer.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/interfaces/ukm_interface.mojom.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"
#include "services/resource_coordinator/public/interfaces/service_callbacks.mojom.h"
#include "services/resource_coordinator/public/interfaces/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "url/gurl.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ResourceCoordinatorWebContentsObserver);

bool ResourceCoordinatorWebContentsObserver::ukm_recorder_initialized = false;

ResourceCoordinatorWebContentsObserver::ResourceCoordinatorWebContentsObserver(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents) {
  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();

  tab_resource_coordinator_ =
      base::MakeUnique<resource_coordinator::ResourceCoordinatorInterface>(
          connector, resource_coordinator::CoordinationUnitType::kWebContents);

  connector->BindInterface(resource_coordinator::mojom::kServiceName,
                           mojo::MakeRequest(&service_callbacks_));

  EnsureUkmRecorderInterface();
}

// TODO(matthalp) integrate into ResourceCoordinatorService once the UKM mojo
// interface can be accessed directly within GRC. GRC cannot currently use
// the UKM mojo inteface directly because of software layering issues
// (i.e. the UKM mojo interface is only reachable from the content_browser
// service which cannot be accesses by services in the services/ directory).
// Instead the ResourceCoordinatorWebContentsObserver is used as it lives
// within the content_browser service and can initialize the
// UkmRecorderInterface and pass that interface into GRC through
// resource_coordinator::mojom::ServiceCallbacks.
void ResourceCoordinatorWebContentsObserver::EnsureUkmRecorderInterface() {
  if (ukm_recorder_initialized) {
    return;
  }

  service_callbacks_->IsUkmRecorderInterfaceInitialized(base::Bind(
      &ResourceCoordinatorWebContentsObserver::MaybeSetUkmRecorderInterface,
      base::Unretained(this)));
}

void ResourceCoordinatorWebContentsObserver::MaybeSetUkmRecorderInterface(
    bool ukm_recorder_already_initialized) {
  if (ukm_recorder_already_initialized) {
    return;
  }

  ukm::mojom::UkmRecorderInterfacePtr ukm_recorder_interface;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(content::mojom::kBrowserServiceName,
                      mojo::MakeRequest(&ukm_recorder_interface));
  service_callbacks_->SetUkmRecorderInterface(
      std::move(ukm_recorder_interface));
  ukm_recorder_initialized = true;
}

ResourceCoordinatorWebContentsObserver::
    ~ResourceCoordinatorWebContentsObserver() = default;

// static
bool ResourceCoordinatorWebContentsObserver::IsEnabled() {
  // Check that service_manager is active and GRC is enabled.
  return content::ServiceManagerConnection::GetForProcess() != nullptr &&
         resource_coordinator::IsResourceCoordinatorEnabled();
}

void ResourceCoordinatorWebContentsObserver::WasShown() {
  tab_resource_coordinator_->SendEvent(
      resource_coordinator::EventType::kOnWebContentsShown);
  tab_resource_coordinator_->SetProperty(
      resource_coordinator::mojom::PropertyType::kVisible,
      base::MakeUnique<base::Value>(true));
}

void ResourceCoordinatorWebContentsObserver::WasHidden() {
  tab_resource_coordinator_->SendEvent(
      resource_coordinator::EventType::kOnWebContentsHidden);
  tab_resource_coordinator_->SetProperty(
      resource_coordinator::mojom::PropertyType::kVisible,
      base::MakeUnique<base::Value>(false));
}

void ResourceCoordinatorWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() || navigation_handle->IsErrorPage() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  if (navigation_handle->IsInMainFrame()) {
    url_ = navigation_handle->GetURL().spec();
    UpdateUkmRecorder();
  }

  content::RenderFrameHost* render_frame_host =
      navigation_handle->GetRenderFrameHost();

  auto* frame_resource_coordinator =
      render_frame_host->GetFrameResourceCoordinator();
  tab_resource_coordinator_->AddChild(*frame_resource_coordinator);

  auto* process_resource_coordinator =
      render_frame_host->GetProcess()->GetProcessResourceCoordinator();
  process_resource_coordinator->AddChild(*frame_resource_coordinator);
}

void ResourceCoordinatorWebContentsObserver::UpdateUkmRecorder() {
  ukm_source_id_ = ukm::UkmRecorder::GetNewSourceID();
  g_browser_process->ukm_recorder()->UpdateSourceURL(ukm_source_id_,
                                                     GURL(url_));
  // ukm::SourceId types need to be converted to a string because base::Value
  // does not guarrantee that its int type will be 64 bits. Instead
  // std:string is used as a canonical format. base::Int64ToString
  // and base::StringToInt64 are used for encoding/decoding respectively.
  tab_resource_coordinator_->SetProperty(
      resource_coordinator::mojom::PropertyType::kUkmSourceId,
      base::MakeUnique<base::Value>(base::Int64ToString(ukm_source_id_)));
}
