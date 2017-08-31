// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_NETWORK_LIST_SORTER_H_
#define CHROMEOS_COMPONENTS_TETHER_NETWORK_LIST_SORTER_H_

#include "base/macros.h"
#include "chromeos/network/network_state_handler.h"
#include "components/cryptauth/remote_device.h"

namespace chromeos {

class NetworkStateHandler;

namespace tether {

// Sorts networks in the order that they should be displayed in network
// settings. The sort order should be determined by network properties,
// prioritized with the following rules:
//   (1) Prefer connected networks to non-connected.
//   (2) Prefer connecting networks to non-connecting.
//   (3) Prefer higher signal strength.
//   (4) Prefer higher battery percentage.
//   (5) Prefer devices which have already connected before.
//   (6) Alphabetize by device name.
//   (7) Alphabetize by cellular carrier name.
//   (8) Alphabetize by GUID.
class NetworkListSorter : public NetworkStateHandler::TetherSortDelegate {
 public:
  NetworkListSorter();
  virtual ~NetworkListSorter();

  // NetworkStateHandler::TetherNetworkListSorter:
  void SortTetherNetworkList(
      NetworkStateHandler::ManagedStateList* tether_networks) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkListSorter);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_NETWORK_LIST_SORTER_H_
