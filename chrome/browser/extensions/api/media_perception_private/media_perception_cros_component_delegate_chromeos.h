// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_CROS_COMPONENT_DELEGATE_CHROMEOS_H_
#define CHROME_BROWSER_EXTENSIONS_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_CROS_COMPONENT_DELEGATE_CHROMEOS_H_

#include "base/callback.h"
#include "extensions/browser/api/media_perception_private/media_perception_cros_component_delegate.h"

namespace extensions {

class MediaPerceptionCrOSComponentDelegateChromeOS
    : public extensions::MediaPerceptionCrOSComponentDelegate {
 public:
  MediaPerceptionCrOSComponentDelegateChromeOS();
  ~MediaPerceptionCrOSComponentDelegateChromeOS() override;

  // extensions::MediaPerceptionCrOSComponentDelegate:
  void LoadCrOSComponent(
      const media_perception::ComponentType& type,
      const base::Callback<void(const std::string&)>& load_callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaPerceptionCrOSComponentDelegateChromeOS);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_CROS_COMPONENT_DELEGATE_CHROMEOS_H_
