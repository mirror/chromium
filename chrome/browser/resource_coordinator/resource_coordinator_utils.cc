// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/resource_coordinator_utils.h"

#include "content/public/common/service_manager_connection.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"

namespace resource_coordinator {

bool IsResourceCoordinatorEnabled() {
  // Check that service_manager is active and the feature is enabled.
  return content::ServiceManagerConnection::GetForProcess() != nullptr &&
         base::FeatureList::IsEnabled(features::kGlobalResourceCoordinator);
}

}  // namespace resource_coordinator
