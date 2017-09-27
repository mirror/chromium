// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/favicon/core/favicon_driver.h"

namespace favicon {

void FaviconDriver::AddObserver(FaviconDriverObserver* observer) {
  observer_list_.AddObserver(observer);
}

void FaviconDriver::RemoveObserver(FaviconDriverObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

FaviconDriver::FaviconDriver() {
}

FaviconDriver::~FaviconDriver() {
}

}  // namespace favicon
