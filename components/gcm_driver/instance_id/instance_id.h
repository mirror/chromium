// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_INSTANCE_ID_INSTANCE_ID_H_
#define COMPONENTS_GCM_DRIVER_INSTANCE_ID_INSTANCE_ID_H_

#include <map>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"

namespace instance_id {

// Encapsulates Instance ID functionalities that need to be implemented for
// different platform. One instance is created per application. Life of
// Instance ID is managed by the InstanceIdDriver.
class InstanceID {
 public:
  enum Result {
    // Successful operation.
    SUCCESS,
    // Invalid parameter.
    INVALID_PARAMETER,
    // Instance ID is disabled.
    DISABLED,
    // Network socket error.
    NETWORK_ERROR,
    // Problem at the server.
    SERVER_ERROR,
    // Other errors.
    UNKNOWN_ERROR
  };

  // Asynchronous callbacks.
  typedef base::Callback<void(InstanceID* instance_id,
                              bool update_id)> TokenRefreshCallback;
  typedef base::Callback<void(InstanceID* instance_id,
                              const std::string& token,
                              Result result)> GetTokenCallback;
  typedef base::Callback<void(InstanceID* instance_id,
                              Result result)> DeleteTokenCallback;
  typedef base::Callback<void(InstanceID* instance_id,
                              Result result)> DeleteIDCallback;

  // Creator.
  // |app_id|: identifies the application that uses the Instance ID.
  // |callback|: to be called when the token refresh is needed.
  static InstanceID* Create(const std::string& app_id);

  virtual ~InstanceID();

  // Sets the callback that will be invoked when the token refresh event needs
  // to be triggered.
  void SetTokenRefreshCallback(const TokenRefreshCallback& callback);

  // Returns the Instance ID.
  virtual std::string GetID() = 0;

  // Returns the time when the InstanceID has been generated.
  virtual base::Time GetCreationTime() = 0;

  // Retrieves a token that allows the identified "audience" to access the
  // service defined as "scope".
  // |audience|: identifies the entity that is authorized to access resources
  //             associated with this Instance ID. It can be another Instance
  //             ID or a project ID.
  // |scope|: identifies authorized actions that the authorized entity can take.
  //          E.g. for sending GCM messages, "GCM" scope should be used.
  // |options|: allows including a small number of string key/value pairs that
  //            will be associated with the token and may be used in processing
  //            the request.
  // |callback|: to be called once the asynchronous operation is done.
  virtual void GetToken(const std::string& audience,
                        const std::string& scope,
                        const std::map<std::string, std::string>& options,
                        const GetTokenCallback& callback) = 0;

  // Revokes a granted token.
  // |audience|: the audience that is passed for obtaining a token.
  // |scope|: the scope that is passed for obtaining a token.
  // |callback|: to be called once the asynchronous operation is done.
  virtual void DeleteToken(const std::string& audience,
                           const std::string& scope,
                           const DeleteTokenCallback& callback) = 0;

  // Resets the app instance identifier and revokes all tokens associated with
  // it.
  // |callback|: to be called once the asynchronous operation is done.
  virtual void DeleteID(const DeleteIDCallback& callback) = 0;

  std::string app_id() const { return app_id_; }

 protected:
  explicit InstanceID(const std::string& app_id);

  void NotifyTokenRefresh(bool update_id);

 private:
  std::string app_id_;
  TokenRefreshCallback token_refresh_callback_;

  DISALLOW_COPY_AND_ASSIGN(InstanceID);
};

}  // namespace instance_id

#endif  // COMPONENTS_GCM_DRIVER_INSTANCE_ID_INSTANCE_ID_H_
