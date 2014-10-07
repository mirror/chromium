// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_EASY_UNLOCK_SERVICE_REGULAR_H_
#define CHROME_BROWSER_SIGNIN_EASY_UNLOCK_SERVICE_REGULAR_H_

#include <string>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/prefs/pref_change_registrar.h"
#include "chrome/browser/signin/easy_unlock_service.h"

namespace base {
class DictionaryValue;
class ListValue;
}

class EasyUnlockToggleFlow;
class Profile;

// EasyUnlockService instance that should be used for regular, non-signin
// profiles.
class EasyUnlockServiceRegular : public EasyUnlockService {
 public:
  explicit EasyUnlockServiceRegular(Profile* profile);
  virtual ~EasyUnlockServiceRegular();

 private:
  // EasyUnlockService implementation.
  virtual EasyUnlockService::Type GetType() const override;
  virtual std::string GetUserEmail() const override;
  virtual void LaunchSetup() override;
  virtual const base::DictionaryValue* GetPermitAccess() const override;
  virtual void SetPermitAccess(const base::DictionaryValue& permit) override;
  virtual void ClearPermitAccess() override;
  virtual const base::ListValue* GetRemoteDevices() const override;
  virtual void SetRemoteDevices(const base::ListValue& devices) override;
  virtual void ClearRemoteDevices() override;
  virtual void RunTurnOffFlow() override;
  virtual void ResetTurnOffFlow() override;
  virtual TurnOffFlowStatus GetTurnOffFlowStatus() const override;
  virtual std::string GetChallenge() const override;
  virtual std::string GetWrappedSecret() const override;
  virtual void InitializeInternal() override;
  virtual void ShutdownInternal() override;
  virtual bool IsAllowedInternal() override;

  // Callback when the controlling pref changes.
  void OnPrefsChanged();

  // Sets the new turn-off flow status.
  void SetTurnOffFlowStatus(TurnOffFlowStatus status);

  // Callback invoked when turn off flow has finished.
  void OnTurnOffFlowFinished(bool success);

  PrefChangeRegistrar registrar_;

  TurnOffFlowStatus turn_off_flow_status_;
  scoped_ptr<EasyUnlockToggleFlow> turn_off_flow_;

  DISALLOW_COPY_AND_ASSIGN(EasyUnlockServiceRegular);
};

#endif  // CHROME_BROWSER_SIGNIN_EASY_UNLOCK_SERVICE_REGULAR_H_
