// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_FAKE_CHROME_IDENTITY_SERVICE_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_FAKE_CHROME_IDENTITY_SERVICE_H_

#include "ios/public/provider/chrome/browser/signin/chrome_identity_service.h"

#include "base/mac/scoped_nsobject.h"
#include "testing/gmock/include/gmock/gmock.h"

@class NSMutableArray;

namespace ios {

// A fake ChromeIdentityService used for testing.
class FakeChromeIdentityService : public ChromeIdentityService {
 public:
  FakeChromeIdentityService();
  ~FakeChromeIdentityService() override;

  // Convenience method that returns the instance of
  // |FakeChromeIdentityService| from the ChromeBrowserProvider.
  static FakeChromeIdentityService* GetInstanceFromChromeProvider();

  // ChromeIdentityService implementation.
  UINavigationController* CreateAccountDetailsController(
      ChromeIdentity* identity,
      id<ChromeIdentityBrowserOpener> browser_opener) override;
  ChromeIdentityInteractionManager* CreateChromeIdentityInteractionManager(
      ios::ChromeBrowserState* browser_state,
      id<ChromeIdentityInteractionManagerDelegate> delegate) const override;

  bool IsValidIdentity(ChromeIdentity* identity) const override;
  ChromeIdentity* GetIdentityWithGaiaID(
      const std::string& gaia_id) const override;
  bool HasIdentities() const override;
  NSArray* GetAllIdentities() const override;
  NSArray* GetAllIdentitiesSortedForDisplay() const override;
  void ForgetIdentity(ChromeIdentity* identity,
                      ForgetIdentityCallback callback) override;

  void GetAccessToken(ChromeIdentity* identity,
                      const std::string& client_id,
                      const std::string& client_secret,
                      const std::set<std::string>& scopes,
                      ios::AccessTokenCallback callback) override;

  void GetAvatarForIdentity(ChromeIdentity* identity,
                            GetAvatarCallback callback) override;

  UIImage* GetCachedAvatarForIdentity(ChromeIdentity* identity) override;

  void GetHostedDomainForIdentity(ChromeIdentity* identity,
                                  GetHostedDomainCallback callback) override;

  ios::MDMDeviceStatus GetMDMDeviceStatus(NSDictionary* user_info) override;

  // Sets the mocked return value for GetMDMDeviceStatus.
  void SetMockMDMDeviceStatus(ios::MDMDeviceStatus mock_status);

  bool HandleMDMNotification(ChromeIdentity* identity,
                             NSDictionary* user_info,
                             ios::MDMStatusCallback callback) override;

  // Counts number of calls to HandleMDMNotification since last reset.
  // You must call ResetHandleMDMNotificationCallCount() first.
  int HandleMDMNotificationCallCount();

  // Resets call count of HandleMDMNotification to 0.
  void ResetHandleMDMNotificationCallCount();

  // Sets up the mock methods for integration tests.
  void SetUpForIntegrationTests();

  // Adds the identities given their name.
  void AddIdentities(NSArray* identitiesNames);

  // Adds |identity| to the available identities.
  void AddIdentity(ChromeIdentity* identity);

  // Removes |identity| from the available identities. No-op if the identity
  // is unknown.
  void RemoveIdentity(ChromeIdentity* identity);

 private:
  base::scoped_nsobject<NSMutableArray> identities_;
  ios::MDMDeviceStatus mock_status_;
  int handle_mdm_notification_call_count_;
};

}  // namespace ios

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_SIGNIN_FAKE_CHROME_IDENTITY_SERVICE_H_
