// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_INSTANCE_ID_INSTANCE_ID_FACTORY_H_
#define COMPONENTS_GCM_DRIVER_INSTANCE_ID_INSTANCE_ID_FACTORY_H_

#include <memory>
#include <string>

namespace gcm {
class GCMDriver;
}

namespace instance_id {

class InstanceID;

class InstanceIDFactory {
 public:
  virtual ~InstanceIDFactory() = default;

  // Creates a new instance of InstanceID. The testing code could override this
  // to provide a mocked instance.
  // |app_id|: identifies the application that uses the Instance ID.
  // |handler|: provides the GCM functionality needed to support Instance ID.
  //            Must outlive this class. On Android, this can be null instead.
  virtual std::unique_ptr<InstanceID> Create(const std::string& app_id,
                                             gcm::GCMDriver* gcm_driver) = 0;
};

}  // namespace instance_id

#endif  // COMPONENTS_GCM_DRIVER_INSTANCE_ID_INSTANCE_ID_FACTORY_H_
