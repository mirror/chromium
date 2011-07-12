// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_BROWSER_POLICY_CONNECTOR_H_
#define CHROME_BROWSER_POLICY_BROWSER_POLICY_CONNECTOR_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/task.h"
#include "chrome/browser/policy/enterprise_install_attributes.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"

class FilePath;
class TestingBrowserProcess;
class TokenService;

namespace policy {

class CloudPolicyDataStore;
class CloudPolicyProvider;
class CloudPolicySubsystem;
class ConfigurationPolicyProvider;
class UserPolicyTokenCache;

// Manages the lifecycle of browser-global policy infrastructure, such as the
// platform policy providers, device- and the user-cloud policy infrastructure.
// TODO(gfeher,mnissler): Factor out device and user specific methods into their
// respective classes.
class BrowserPolicyConnector : public NotificationObserver {
 public:
  static BrowserPolicyConnector* Create();
  virtual ~BrowserPolicyConnector();

  ConfigurationPolicyProvider* GetManagedPlatformProvider() const;
  ConfigurationPolicyProvider* GetManagedCloudProvider() const;
  ConfigurationPolicyProvider* GetRecommendedPlatformProvider() const;
  ConfigurationPolicyProvider* GetRecommendedCloudProvider() const;

  // Returns a weak pointer to the CloudPolicySubsystem corresponding to the
  // device policy managed by this policy connector, or NULL if no such
  // subsystem exists (i.e. when running outside ChromeOS).
  CloudPolicySubsystem* device_cloud_policy_subsystem() {
#if defined(OS_CHROMEOS)
    return device_cloud_policy_subsystem_.get();
#else
    return NULL;
#endif
  }

  // Returns a weak pointer to the CloudPolicySubsystem corresponding to the
  // user policy managed by this policy connector, or NULL if no such
  // subsystem exists (i.e. when user cloud policy is not active due to
  // unmanaged or not logged in).
  CloudPolicySubsystem* user_cloud_policy_subsystem() {
    return user_cloud_policy_subsystem_.get();
  }

  // Used to set the credentials stored in the identity strategy associated
  // with this policy connector.
  void SetDeviceCredentials(const std::string& owner_email,
                            const std::string& gaia_token);

  // Returns true if this device is managed by an enterprise (as opposed to
  // a local owner).
  bool IsEnterpriseManaged();

  // Locks the device to an enterprise domain.
  EnterpriseInstallAttributes::LockResult LockDevice(const std::string& user);

  // Returns the enterprise domain if device is managed.
  std::string GetEnterpriseDomain();

  // Exposes the StopAutoRetry() method of the CloudPolicySubsystem managed
  // by this connector, which can be used to disable automatic
  // retrying behavior.
  void DeviceStopAutoRetry();

  // Initiates a policy fetch after a successful device registration.
  void FetchDevicePolicy();

  // Schedules initialization of the cloud policy backend services, if the
  // services are already constructed.
  void ScheduleServiceInitialization(int64 delay_milliseconds);

  // Initializes the user cloud policy infrasturcture.
  // TODO(sfeuz): Listen to log-out or going-away messages of TokenService and
  // reset the backend at that point.
  void InitializeUserPolicy(const std::string& user_name,
                            const FilePath& policy_dir,
                            TokenService* token_service);

 private:
  friend class ::TestingBrowserProcess;

  BrowserPolicyConnector();

  static BrowserPolicyConnector* CreateForTests();
  static ConfigurationPolicyProvider* CreateManagedPlatformProvider();
  static ConfigurationPolicyProvider* CreateRecommendedPlatformProvider();

  // Constructor for tests that allows tests to use fake platform and cloud
  // policy providers instead of using the actual ones.
  BrowserPolicyConnector(
      ConfigurationPolicyProvider* managed_platform_provider,
      ConfigurationPolicyProvider* recommended_platform_provider,
      CloudPolicyProvider* managed_cloud_provider,
      CloudPolicyProvider* recommended_cloud_provider);

  // NotificationObserver method overrides:
  virtual void Observe(int type,
                       const NotificationSource& source,
                       const NotificationDetails& details) OVERRIDE;

  // Initializes the device cloud policy infrasturcture.
  void InitializeDevicePolicy();

  // Activates the device cloud policy subsystem. This will be posted as a task
  // from InitializeDevicePolicy since it needs to wait for the message loops to
  // be running.
  void InitializeDevicePolicySubsystem();

  scoped_ptr<ConfigurationPolicyProvider> managed_platform_provider_;
  scoped_ptr<ConfigurationPolicyProvider> recommended_platform_provider_;

  scoped_ptr<CloudPolicyProvider> managed_cloud_provider_;
  scoped_ptr<CloudPolicyProvider> recommended_cloud_provider_;

#if defined(OS_CHROMEOS)
  scoped_ptr<CloudPolicyDataStore> device_data_store_;
  scoped_ptr<CloudPolicySubsystem> device_cloud_policy_subsystem_;
  scoped_ptr<EnterpriseInstallAttributes> install_attributes_;
#endif

  scoped_refptr<UserPolicyTokenCache> user_policy_token_cache_;
  scoped_ptr<CloudPolicyDataStore> user_data_store_;
  scoped_ptr<CloudPolicySubsystem> user_cloud_policy_subsystem_;

  ScopedRunnableMethodFactory<BrowserPolicyConnector> method_factory_;

  // Registers the provider for notification of successful Gaia logins.
  NotificationRegistrar registrar_;

  // Weak reference to the TokenService we are listening to for user cloud
  // policy authentication tokens.
  TokenService* token_service_;

  DISALLOW_COPY_AND_ASSIGN(BrowserPolicyConnector);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_BROWSER_POLICY_CONNECTOR_H_
