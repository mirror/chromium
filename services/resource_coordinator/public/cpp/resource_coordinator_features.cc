// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

#include "content/public/common/service_manager_connection.h"

namespace features {

// Globally enable the GRC.
// When checking if GRC is enabled use |IsResourceCoordinatorEnabled| instead
// of |base::FeatureList::IsEnabled(features::kGlobalResourceCoordinator)|.
const base::Feature kGlobalResourceCoordinator{
    "GlobalResourceCoordinator", base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace features

namespace resource_coordinator {

bool IsResourceCoordinatorEnabled() {
  // Check that service_manager is active and the feature is enabled.
  return content::ServiceManagerConnection::GetForProcess() != nullptr &&
         base::FeatureList::IsEnabled(features::kGlobalResourceCoordinator);
}

}  // namespace resource_coordinator
