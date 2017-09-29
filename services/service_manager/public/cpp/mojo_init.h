// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_PUBLIC_CPP_MOJO_INIT_H_
#define SERVICES_SERVICE_MANAGER_PUBLIC_CPP_MOJO_INIT_H_

#include "services/service_manager/public/cpp/export.h"

namespace service_manager {

// Perform any necessary Mojo initialization.
SERVICE_MANAGER_PUBLIC_CPP_EXPORT void InitializeMojo();

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_PUBLIC_CPP_MOJO_INIT_H_
