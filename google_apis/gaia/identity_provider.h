// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_IDENTITY_PROVIDER_H_
#define GOOGLE_APIS_GAIA_IDENTITY_PROVIDER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "google_apis/gaia/oauth2_token_service.h"

// Helper class that provides access to information about logged-in GAIA
// accounts. Each instance of this class references an entity who may be logged
// in to zero, one or multiple GAIA accounts. The class provides access to the
// OAuth tokens for all logged-in accounts and indicates which of these is
// currently active.
// The main purpose of this abstraction layer is to isolate consumers of GAIA
// information from the different sources and various token service
// implementations. Whenever possible, consumers of GAIA information should be
// provided with an instance of this class instead of accessing other GAIA APIs
// directly.
class IdentityProvider : public OAuth2TokenService::Observer {
 public:
  // Observer class for events related to the active account ID. The order of
  // notification is guaranteed:
  // * |OnActiveAccountLogin| is always called before
  class Observer {
   public:
    // Called when a GAIA account logs in and becomes the active account. All
    // account information is available when this method is called and all
    // |IdentityProvider| methods will return valid data.
    virtual void OnActiveAccountLogin() {}

    // Called when a refresh token is available for the active account.
    //
    // During a sign-in flow, this method is guaranteed to be called only after
    // |OnActiveAccountLogin| is called.
    virtual void OnRefreshTokenAvailableForActiveAccount() {}

    // Called when the refresh token of the active account is revoked.
    //
    // During a sign-in flow, this method is guaranteed to be called only after
    // |OnActiveAccountLogin| is called.
    virtual void OnRefreshTokenRevokedForActiveAccount() {}

    // Called when the active GAIA account logs out. The account information may
    // have been cleared already when this method is called. The
    // |IdentityProvider| methods may return inconsistent or outdated
    // information if called from within OnLogout().
    virtual void OnActiveAccountLogout() {}

   protected:
    virtual ~Observer();
  };

  ~IdentityProvider() override;

  // Gets the active account's user name.
  virtual std::string GetActiveUsername() = 0;

  // Gets the active account's account ID.
  virtual std::string GetActiveAccountId() = 0;

  // Gets the token service vending OAuth tokens for all logged-in accounts.
  OAuth2TokenService* GetTokenService() { return token_service_; };

  // Requests login to a GAIA account. Implementations can show a login UI, log
  // in automatically if sufficient credentials are available or may ignore the
  // request. Returns true if the login request was processed and false if it
  // was ignored.
  virtual bool RequestLogin() = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // OAuth2TokenService::Observer:
  void OnRefreshTokenAvailable(const std::string& account_id) override;
  void OnRefreshTokenRevoked(const std::string& account_id) override;

 protected:
  IdentityProvider(OAuth2TokenService* token_service);

  // Fires an OnActiveAccountLogin notification.
  void FireOnActiveAccountLogin();

  // Fires an OnActiveAccountLogout notification.
  void FireOnActiveAccountLogout();

 private:
  // Weak reference to the token service. Must outlive this.
  OAuth2TokenService* token_service_;

  base::ObserverList<Observer, true> observers_;

  DISALLOW_COPY_AND_ASSIGN(IdentityProvider);
};

#endif  // GOOGLE_APIS_GAIA_IDENTITY_PROVIDER_H_
