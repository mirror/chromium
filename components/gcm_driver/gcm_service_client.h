// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_MOJO_GCM_SERVICE_CLIENT_H_
#define COMPONENTS_GCM_DRIVER_MOJO_GCM_SERVICE_CLIENT_H_

namespace gcm {

class GCMServiceClient {
 public:
  virtual PreStart() = 0;
};

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_MOJO_GCM_SERVICE_CLIENT_H_
