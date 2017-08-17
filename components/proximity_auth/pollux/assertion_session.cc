// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/pollux/assertion_session.h"

namespace pollux {

AssertionSession::AssertionSession(const Parameters& parameters)
    : parameters_(parameters), state_(State::NOT_STARTED) {}

AssertionSession::~AssertionSession() {}

void AssertionSession::Start() {
  DCHECK(state_ == State::NOT_STARTED);
  // 1. Advertise
  // 2. Wait for BLE connection
  // 3. Negotiate uWeave server socket
  // 4. Create SecureChannel over socket (synchronous)
  //    --- wait for screenlock? ---
  // 5. Send challenge
  // 6. Receive assertion
}

void AssertionSession::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AssertionSession::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace pollux
