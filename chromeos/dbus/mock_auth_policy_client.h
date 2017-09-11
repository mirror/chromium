// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_MOCK_AUTH_POLICY_CLIENT_H_
#define CHROMEOS_DBUS_MOCK_AUTH_POLICY_CLIENT_H_

#include "chromeos/dbus/auth_policy_client.h"
#include "components/signin/core/account_id/account_id.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

class MockAuthPolicyClient : public AuthPolicyClient {
 public:
  using AuthCallback = AuthPolicyClient::AuthCallback;
  using JoinCallback = AuthPolicyClient::JoinCallback;

  MockAuthPolicyClient();
  virtual ~MockAuthPolicyClient();

  MOCK_METHOD1(Init, void(dbus::Bus* dbus));

  // GMock can not handle non copyable objects (which base::OnceCallback are).
  // So we have helper functions Do*.
  void JoinAdDomain(const std::string& machine_name,
                    const std::string& user_principal_name,
                    int password_fd,
                    JoinCallback callback) override {
    DoJoinAdDomain(machine_name, user_principal_name, password_fd, &callback);
  }
  MOCK_METHOD4(DoJoinAdDomain,
               void(const std::string& machine_name,
                    const std::string& user_principal_name,
                    int password_fd,
                    JoinCallback* callback));

  void AuthenticateUser(const std::string& user_principal_name,
                        const std::string& object_guid,
                        int password_fd,
                        AuthCallback callback) override {
    DoAuthenticateUser(user_principal_name, object_guid, password_fd,
                       &callback);
  }
  MOCK_METHOD4(DoAuthenticateUser,
               void(const std::string& user_principal_name,
                    const std::string& object_guid,
                    int password_fd,
                    AuthCallback* callback));

  void GetUserStatus(const std::string& object_guid,
                     GetUserStatusCallback callback) override {
    DoGetUserStatus(object_guid, &callback);
  }
  MOCK_METHOD2(DoGetUserStatus,
               void(const std::string& object_guid,
                    GetUserStatusCallback* callback));

  void GetUserKerberosFiles(const std::string& object_guid,
                            GetUserKerberosFilesCallback callback) override {
    DoGetUserKerberosFiles(object_guid, &callback);
  }
  MOCK_METHOD2(DoGetUserKerberosFiles,
               void(const std::string& object_guid,
                    GetUserKerberosFilesCallback* callback));

  void RefreshDevicePolicy(RefreshPolicyCallback callback) override {
    DoRefreshDevicePolicy(&callback);
  }
  MOCK_METHOD1(DoRefreshDevicePolicy, void(RefreshPolicyCallback* callback));

  void RefreshUserPolicy(const AccountId& account_id,
                         RefreshPolicyCallback callback) {
    DoRefreshUserPolicy(account_id, &callback);
  }
  MOCK_METHOD2(DoRefreshUserPolicy,
               void(const AccountId& account_id,
                    RefreshPolicyCallback* callback));

  MOCK_METHOD3(
      ConnectToSignal,
      void(const std::string& signal_name,
           dbus::ObjectProxy::SignalCallback signal_callback,
           dbus::ObjectProxy::OnConnectedCallback on_connected_callback));
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_MOCK_AUTH_POLICY_CLIENT_H_
