// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/disconnect_tethering_request_sender.h"

namespace chromeos {

namespace tether {

class FakeDisconnectTetheringRequestSender
    : public DisconnectTetheringRequestSender {
 public:
  FakeDisconnectTetheringRequestSender();
  ~FakeDisconnectTetheringRequestSender();

  void SendDisconnectRequestToDevice(const std::string& device_id) override;

  std::vector<std::string> device_ids() { return device_ids_; }

 private:
  std::vector<std::string> device_ids_;
};

}  // namespace tether

}  // namespace chromeos
