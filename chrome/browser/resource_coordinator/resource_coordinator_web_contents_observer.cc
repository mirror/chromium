// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/resource_coordinator_web_contents_observer.h"

#include <utility>

#include "base/atomic_sequence_num.h"
#include "base/feature_list.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/resource_coordinator/tab_manager_grc_tab_signal_observer.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/interfaces/coordination_unit.mojom.h"
#include "services/resource_coordinator/public/interfaces/service_callbacks.mojom.h"
#include "services/resource_coordinator/public/interfaces/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ResourceCoordinatorWebContentsObserver);

ResourceCoordinatorWebContentsObserver::ResourceCoordinatorWebContentsObserver(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents) {
  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();

  page_resource_coordinator_ =
      base::MakeUnique<resource_coordinator::ResourceCoordinatorInterface>(
          connector, resource_coordinator::CoordinationUnitType::kPage);

  // Make sure to set the visibility property when we create
  // |page_resource_coordinator_|.
  page_resource_coordinator_->SetProperty(
      resource_coordinator::mojom::PropertyType::kVisible,
      web_contents->IsVisible());

#if !defined(OS_ANDROID)
  if (auto* grc_tab_signal_observer = resource_coordinator::TabManager::
          GRCTabSignalObserver::GetInstance()) {
    // Gets CoordinationUnitID for this WebContents and adds it to
    // GRCTabSignalObserver.
    grc_tab_signal_observer->AssociateCoordinationUnitIDWithWebContents(
        page_resource_coordinator_->id(), web_contents);
  }
#endif
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
  page_resource_coordinator_->SetProperty(
      resource_coordinator::mojom::PropertyType::kVisible, true);
}

void ResourceCoordinatorWebContentsObserver::WasHidden() {
  page_resource_coordinator_->SetProperty(
      resource_coordinator::mojom::PropertyType::kVisible, false);
}

void ResourceCoordinatorWebContentsObserver::WebContentsDestroyed() {
#if !defined(OS_ANDROID)
  if (auto* grc_tab_signal_observer = resource_coordinator::TabManager::
          GRCTabSignalObserver::GetInstance()) {
    // Gets CoordinationUnitID for this WebContents and removes it from
    // GRCTabSignalObserver.
    grc_tab_signal_observer->RemoveCoordinationUnitID(
        page_resource_coordinator_->id());
  }
#endif
}

void ResourceCoordinatorWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() || navigation_handle->IsErrorPage() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  if (navigation_handle->IsInMainFrame()) {
    UpdateUkmRecorder(navigation_handle->GetNavigationId());
    ResetFlag();
    page_resource_coordinator_->SendEvent(
        resource_coordinator::mojom::Event::kNavigationCommitted);
  }

  content::RenderFrameHost* render_frame_host =
      navigation_handle->GetRenderFrameHost();

  auto* frame_resource_coordinator =
      render_frame_host->GetFrameResourceCoordinator();
  page_resource_coordinator_->AddChild(*frame_resource_coordinator);

  auto* process_resource_coordinator =
      render_frame_host->GetProcess()->GetProcessResourceCoordinator();
  process_resource_coordinator->AddChild(*frame_resource_coordinator);
}

void ResourceCoordinatorWebContentsObserver::TitleWasSet(
    content::NavigationEntry* entry) {
  if (!first_time_title_set_) {
    first_time_title_set_ = true;
    return;
  }
  page_resource_coordinator_->SendEvent(
      resource_coordinator::mojom::Event::kTitleUpdated);
}

void ResourceCoordinatorWebContentsObserver::DidUpdateFaviconURL(
    const std::vector<content::FaviconURL>& candidates) {
  if (!first_time_favicon_set_) {
    first_time_favicon_set_ = true;
    return;
  }
  page_resource_coordinator_->SendEvent(
      resource_coordinator::mojom::Event::kFaviconUpdated);
}

void ResourceCoordinatorWebContentsObserver::UpdateUkmRecorder(
    int64_t navigation_id) {
  ukm_source_id_ =
      ukm::ConvertToSourceId(navigation_id, ukm::SourceIdType::NAVIGATION_ID);
  page_resource_coordinator_->SetProperty(
      resource_coordinator::mojom::PropertyType::kUKMSourceId, ukm_source_id_);
}

void ResourceCoordinatorWebContentsObserver::ResetFlag() {
  first_time_title_set_ = false;
  first_time_favicon_set_ = false;
}
