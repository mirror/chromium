// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/instance_id/instance_id_impl_factory.h"

#include "base/memory/ptr_util.h"
#include "components/gcm_driver/instance_id/instance_id_android.h"

namespace instance_id {

std::unique_ptr<InstanceID> InstanceIDImplFactory::Create(
    const std::string& app_id,
    gcm::GCMDriver* gcm_driver) {
  return base::MakeUnique<InstanceIDAndroid>(app_id, gcm_driver);
}

}  // namespace instance_id
