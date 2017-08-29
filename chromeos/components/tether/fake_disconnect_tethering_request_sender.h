// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "chromeos/components/tether/disconnect_tethering_request_sender.h"

namespace chromeos {

namespace tether {

// Test double for DisconnectTetheringRequestSender
class FakeDisconnectTetheringRequestSender
    : public DisconnectTetheringRequestSender {
 public:
  FakeDisconnectTetheringRequestSender();
  ~FakeDisconnectTetheringRequestSender();

  // DisconnectTetheringRequestSender:
  void SendDisconnectRequestToDevice(const std::string& device_id) override;

  std::vector<std::string> device_ids_sent_requests() {
    return device_ids_sent_requests_;
  }

 private:
  std::vector<std::string> device_ids_sent_requests_;
};

}  // namespace tether

}  // namespace chromeos
