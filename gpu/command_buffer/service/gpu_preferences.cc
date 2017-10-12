// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/gpu_preferences.h"

#include <cstring>

#include "base/base64.h"

namespace gpu {

GpuPreferences::GpuPreferences() = default;

GpuPreferences::GpuPreferences(const GpuPreferences& other) = default;

GpuPreferences::~GpuPreferences() {}

std::string GpuPreferences::ToString() const {
  std::string encoded;
  base::Base64Encode(
      base::StringPiece(reinterpret_cast<const char*>(this), sizeof(*this)),
      &encoded);
  return encoded;
}

// static
bool GpuPreferences::FromString(const std::string& data,
                                GpuPreferences* output_prefs) {
  DCHECK(output_prefs);
  std::string decoded;
  if (!base::Base64Decode(data, &decoded))
    return false;
  DCHECK_EQ(decoded.size(), sizeof(*output_prefs));
  memcpy(output_prefs, decoded.data(), decoded.size());
  return true;
}

}  // namespace gpu
