// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/content/renovations/web_contents_loading_observer.h"

#include <utility>

#include "base/logging.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/interfaces/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(offline_pages::WebContentsLoadingObserver);

namespace offline_pages {

WebContentsLoadingObserver::WebContentsLoadingObserver(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      web_contents_(web_contents),
      binding_(this) {
  DCHECK(web_contents_);

  // Create resource coordinator interface for WebContents.
  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  DCHECK(connector);
  resource_coordinator_ =
      base::MakeUnique<resource_coordinator::ResourceCoordinatorInterface>(
          connector, resource_coordinator::CoordinationUnitType::kWebContents);

  // Listen for tab-level events.
  resource_coordinator::mojom::TabSignalGeneratorPtr tab_signal_generator_ptr;
  connector->BindInterface(resource_coordinator::mojom::kServiceName,
                           mojo::MakeRequest(&tab_signal_generator_ptr));
  resource_coordinator::mojom::TabSignalObserverPtr tab_signal_observer_ptr;
  binding_.Bind(mojo::MakeRequest(&tab_signal_observer_ptr));
  tab_signal_generator_ptr->AddObserver(std::move(tab_signal_observer_ptr));

  DLOG(INFO) << "COLLINBAKER Registered TabSignalObserver.";
}

WebContentsLoadingObserver::~WebContentsLoadingObserver() = default;

void WebContentsLoadingObserver::OnEventReceived(
    const resource_coordinator::CoordinationUnitID& id,
    resource_coordinator::mojom::TabEvent tab_event) {
  if (tab_event == resource_coordinator::mojom::TabEvent::kDoneLoading &&
      id == resource_coordinator_->id()) {
    DLOG(INFO) << "COLLINBAKER kDoneLoading received.";
  }
}

void WebContentsLoadingObserver::DocumentOnLoadCompletedInMainFrame() {
  DLOG(INFO) << "COLLINBAKER DocumentOnLoadCompletedInMainFrame received.";
}

void WebContentsLoadingObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() || navigation_handle->IsErrorPage() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  // Add new frame to the resource coordinator graph.

  content::RenderFrameHost* render_frame_host =
      navigation_handle->GetRenderFrameHost();

  auto* frame_resource_coordinator =
      render_frame_host->GetFrameResourceCoordinator();
  resource_coordinator_->AddChild(*frame_resource_coordinator);

  auto* process_resource_coordinator =
      render_frame_host->GetProcess()->GetProcessResourceCoordinator();
  process_resource_coordinator->AddChild(*frame_resource_coordinator);
}

}  // namespace offline_pages
