// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/vr_assets.h"

#include "base/memory/singleton.h"

namespace vr {

struct VrAssetsSingletonTrait : public base::DefaultSingletonTraits<VrAssets> {
  static VrAssets* New() { return new VrAssets(); }
};

// static
VrAssets* VrAssets::GetInstance() {
  return base::Singleton<VrAssets, VrAssetsSingletonTrait>::get();
}

VrAssets::~VrAssets() = default;

void VrAssets::OnComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  // TODO(706150): Make assets available to VR.
}

VrAssets::VrAssets() = default;

}  // namespace vr
