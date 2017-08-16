// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/public/multidevice_service.h"

class MultiDeviceTestService : public multidevice::MultiDeviceService {
 public:
  explicit MultiDeviceTestService() {}
  ~MultiDeviceService() override {}
}