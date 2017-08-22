// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PROXIMITY_AUTH_POLLUX_ASSERTION_SESSION_H
#define COMPONENTS_PROXIMITY_AUTH_POLLUX_ASSERTION_SESSION_H

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/proximity_auth/pollux/ble_scanner.h"

namespace pollux {

// Contains the entire state of a session to get an assertion from the remote
// authenticator. That is to say, an instance of this class should be created
// when navigator.credentials.get() is called for a credential backed by a
// Pollux authenticator.
class AssertionSession {
 public:
  enum State {
    NOT_STARTED,
    ADVERTISING,
    CONNECTED,
    COMPLETED_WITH_CHALLENGE,
    TIMED_OUT,
    ERROR,
  };

  struct Parameters {
    std::string eid;
    std::string session_key;
    std::string challenge;
    base::TimeDelta timeout;
  };

  class Observer {
   public:
    virtual ~Observer() {}
    virtual void OnStateChanged(State old_state, State new_state) = 0;
    virtual void OnGotAssertion(const std::string& assertion) = 0;
    virtual void OnError(const std::string& error_message) = 0;
  };

  explicit AssertionSession(const Parameters& parameters);
  virtual ~AssertionSession();

  void Start();

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  State state() const { return state_; }

 private:
  void SetState(State state);

  // Callback to be called when the Bluetooth adapter is initialized.
  void OnAdapterInitialized(scoped_refptr<device::BluetoothAdapter> adapter);

  void OnAdvertisementRegistered(
      scoped_refptr<device::BluetoothAdvertisement> advertisement);

  void OnAdvertisementError(
      device::BluetoothAdvertisement::ErrorCode error_code);

  void OnConnectionFound(std::unique_ptr<cryptauth::Connection> connection);

  Parameters parameters_;
  State state_;
  base::ObserverList<Observer> observers_;

  scoped_refptr<device::BluetoothAdapter> adapter_;
  std::unique_ptr<BleScanner> ble_scanner_;

  // base::Timer timer_;
  // Advertiser advertiser_:
  // SecureChannel secure_channel_;

  base::WeakPtrFactory<AssertionSession> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AssertionSession);
};

}  // namespace pollux

#endif  // COMPONENTS_PROXIMITY_AUTH_POLLUX_ASSERTION_SESSION_H
