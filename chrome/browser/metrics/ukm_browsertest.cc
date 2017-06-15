// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/single_client_status_change_checker.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/ukm/public/ukm_recorder.h"
#include "components/ukm/ukm_service.h"

using browser_sync::ProfileSyncService;

namespace metrics {

// bool HasAuthError(ProfileSyncService* service) {
//   return service->GetAuthError().state() ==
//              GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS ||
//          service->GetAuthError().state() ==
//              GoogleServiceAuthError::SERVICE_ERROR ||
//          service->GetAuthError().state() ==
//              GoogleServiceAuthError::REQUEST_CANCELED;
// }

// class EngineInitializeChecker : public SingleClientStatusChangeChecker {
//  public:
//   explicit EngineInitializeChecker(ProfileSyncService* service)
//       : SingleClientStatusChangeChecker(service) {}

//   bool IsExitConditionSatisfied() override {
//     if (service()->IsEngineInitialized())
//       return true;
//     // Engine initialization is blocked by an auth error.
//     if (HasAuthError(service()))
//       return true;
//     // Engine initialization is blocked by a failure to fetch Oauth2 tokens.
//     if (service()->IsRetryingAccessTokenFetchForTest())
//       return true;
//     // Still waiting on engine initialization.
//     return false;
//   }

//   std::string GetDebugMessage() const override { return "Engine Initialize";
//   }
// };

// Test fixture that provides access to some UKM internals.
class UkmBrowserTest : public InProcessBrowserTest {
 public:
  UkmBrowserTest() : InProcessBrowserTest() {}

 protected:
  bool ukm_enabled() {
    auto* service = ukm_service();
    return service ? service->recording_enabled_ : false;
  }
  uint64_t client_id() {
    auto* service = ukm_service();
    return service ? service->client_id_ : 0;
  }

 private:
  ukm::UkmService* ukm_service() {
    return static_cast<ukm::UkmService*>(ukm::UkmRecorder::Get());
  }
};

IN_PROC_BROWSER_TEST_F(UkmBrowserTest, IncognitoCheck) {
  LOG(ERROR) << "Enabling metrics";
  bool metrics_enabled = true;
  ChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(
      &metrics_enabled);
  g_browser_process->GetMetricsServicesManager()->UpdateUploadPermissions(
      false);

  // browser_sync::ProfileSyncService* sync =
  //     ProfileSyncServiceFactory::GetInstance()->GetForProfile(
  //         ProfileManager::GetActiveUserProfile());
  // if (!EngineInitializeChecker(sync).Wait()) {
  //   LOG(ERROR) << "EngineInitializeChecker timed out.";
  // }

  CreateBrowser(ProfileManager::GetActiveUserProfile());
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();

  Browser* incognito_browser = CreateIncognitoBrowser();
  EXPECT_FALSE(ukm_enabled());

  CloseBrowserSynchronously(incognito_browser);
  EXPECT_TRUE(ukm_enabled());
  // Client ID should not have been reset.
  EXPECT_EQ(original_client_id, client_id());

  ChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(nullptr);
}

}  // namespace metrics
