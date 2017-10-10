// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_discovery.h"

#include <utility>

#include "base/logging.h"

namespace device {

U2fDiscovery::U2fDiscovery() = default;

U2fDiscovery::~U2fDiscovery() = default;

void U2fDiscovery::SetObserver(base::WeakPtr<Observer> observer) {
  observer_ = std::move(observer);
}

void U2fDiscovery::ResetObserver() {
  observer_.reset();
}

}  // namespace device
