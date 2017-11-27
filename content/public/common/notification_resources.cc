// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/notification_resources.h"

#include "base/logging.h"

namespace content {

NotificationResources::NotificationResources() {
  LOG(WARNING) << "ANITA: " << __FUNCTION__;
}

NotificationResources::NotificationResources(
    const NotificationResources& other) = default;

NotificationResources::~NotificationResources() {}

}  // namespace content
