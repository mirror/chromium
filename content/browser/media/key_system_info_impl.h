// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_KEY_SYSTEM_INFO_IMPL_H_
#define CONTENT_BROWSER_MEDIA_KEY_SYSTEM_INFO_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/public/common/cdm_info.h"
#include "media/mojo/interfaces/key_system_info.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

// This class implements the media::mojom::KeySystemInfo interface.
class KeySystemInfoImpl final : public media::mojom::KeySystemInfo {
 public:
  // Create a KeySystemInfoImpl object and bind it to |request|.
  static void Create(media::mojom::KeySystemInfoRequest request);

  // Returns CdmInfo registered for |key_system|. Returns null if no CdmInfo is
  // registered for |key_system|, or if the CdmInfo registered is invalid.
  static std::unique_ptr<CdmInfo> GetCdmInfoForKeySystem(
      const std::string& key_system);

  // media::mojom::KeySystemInfo implementation.
  void IsKeySystemAvailable(const std::string& key_system,
                            IsKeySystemAvailableCallback callback) final;

 private:
  KeySystemInfoImpl(media::mojom::KeySystemInfoRequest request);
  ~KeySystemInfoImpl() final;

  mojo::Binding<media::mojom::KeySystemInfo> binding_;

  DISALLOW_COPY_AND_ASSIGN(KeySystemInfoImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_KEY_SYSTEM_INFO_IMPL_H_
