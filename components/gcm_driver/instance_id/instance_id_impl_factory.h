// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_INSTANCE_ID_INSTANCE_ID_IMPL_FACTORY_H_
#define COMPONENTS_GCM_DRIVER_INSTANCE_ID_INSTANCE_ID_IMPL_FACTORY_H_

#include "base/macros.h"
#include "components/gcm_driver/instance_id/instance_id_factory.h"

namespace instance_id {

class InstanceIDImplFactory : public InstanceIDFactory {
 public:
  InstanceIDImplFactory() = default;
  ~InstanceIDImplFactory() override = default;

  std::unique_ptr<InstanceID> Create(const std::string& app_id,
                                     gcm::GCMDriver* gcm_driver) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(InstanceIDImplFactory);
};

}  // namespace instance_id

#endif  // COMPONENTS_GCM_DRIVER_INSTANCE_ID_INSTANCE_ID_IMPL_FACTORY_H_
