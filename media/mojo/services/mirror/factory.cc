// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mirror/factory.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "media/mojo/services/mirror/mirror_service.h"

namespace media {

std::unique_ptr<service_manager::Service> CreateMirrorService() {
  LOG(INFO) << "CreateMirrorService.";
  return std::unique_ptr<service_manager::Service>(new MirrorService());
}

}  // namespace media
