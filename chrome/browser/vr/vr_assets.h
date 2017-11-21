// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_ASSETS_H_
#define CHROME_BROWSER_VR_VR_ASSETS_H_

#include <stdint.h>
#include <memory>

namespace base {
class DictionaryValue;
class FilePath;
class Version;
}  // namespace base

namespace vr {

constexpr uint32_t kCompatibleMajorVrAssetsComponentVersion = 0;

struct VrAssetsSingletonTrait;

// Manages VR assets such as the environment. Gets updated by
// the VR assets component.
class VrAssets {
 public:
  static VrAssets* GetInstance();  // thread-safe.

  ~VrAssets();

  void OnComponentReady(const base::Version& version,
                        const base::FilePath& install_dir,
                        std::unique_ptr<base::DictionaryValue> manifest);

 private:
  VrAssets();

  friend struct VrAssetsSingletonTrait;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_ASSETS_H_
