// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/media_service_factory.h"

#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "media/mojo/services/media_service.h"
#include "media/mojo/services/test_mojo_media_client.h"

#if defined(OS_ANDROID)
#include "media/mojo/services/android_mojo_media_client.h"  // nogncheck
#endif

namespace media {

// This must be in a separate file from the other media service factories to
// avoid pulling in test code to non-test binaries.
std::unique_ptr<service_manager::Service> CreateMediaServiceForTesting() {
  return std::unique_ptr<service_manager::Service>(
      new MediaService(base::MakeUnique<TestMojoMediaClient>()));
}

}  // namespace media
