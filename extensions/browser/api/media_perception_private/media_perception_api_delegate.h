// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_API_DELEGATE_H_
#define EXTENSIONS_BROWSER_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_API_DELEGATE_H_

#include "base/callback.h"
#include "extensions/common/api/media_perception_private.h"

namespace media_perception = extensions::api::media_perception_private;

namespace extensions {

class MediaPerceptionAPIDelegate {
 public:
  virtual ~MediaPerceptionAPIDelegate() {}

  // Provides an interface through which a media analytics Chrome OS component
  // from Component Updater can be loaded and mounted on a device.
  virtual void LoadCrOSComponent(
      const media_perception::ComponentType& type,
      base::OnceCallback<void(const std::string&)> load_callback) = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_API_DELEGATE_H_
