// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/key_system_info_impl.h"

#include <vector>

#include "base/logging.h"
#include "content/public/browser/cdm_registry.h"
#include "content/public/common/cdm_info.h"
#include "media/base/key_system_names.h"

namespace content {

// static
void KeySystemInfoImpl::Create(media::mojom::KeySystemInfoRequest request) {
  DVLOG(3) << __func__;
  // The created object is bound to (and owned by) |request|.
  new KeySystemInfoImpl(std::move(request));
}

// static
std::unique_ptr<CdmInfo> KeySystemInfoImpl::GetCdmInfoForKeySystem(
    const std::string& key_system) {
  DVLOG(2) << __func__ << ": key_system = " << key_system;
  for (const auto& cdm : CdmRegistry::GetInstance()->GetAllRegisteredCdms()) {
    if (cdm.supported_key_system == key_system ||
        (cdm.supports_sub_key_systems &&
         media::IsChildKeySystemOf(key_system, cdm.supported_key_system))) {
      return std::make_unique<CdmInfo>(cdm);
    }
  }

  return nullptr;
}

KeySystemInfoImpl::KeySystemInfoImpl(media::mojom::KeySystemInfoRequest request)
    : binding_(this, std::move(request)) {}

KeySystemInfoImpl::~KeySystemInfoImpl() {}

void KeySystemInfoImpl::IsKeySystemAvailable(
    const std::string& key_system,
    IsKeySystemAvailableCallback callback) {
  DVLOG(3) << __func__;
  std::unique_ptr<CdmInfo> cdm = GetCdmInfoForKeySystem(key_system);
  if (!cdm) {
    std::move(callback).Run(false, {}, false);
    return;
  }

  std::move(callback).Run(true, cdm->supported_codecs,
                          cdm->supports_persistent_license);
}

}  // namespace content
