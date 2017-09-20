// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "string_resource.h"

#include "ui/base/l10n/l10n_util.h"

namespace vr {

base::string16 GetStringResource(int string_id) {
  return l10n_util::GetStringUTF16(string_id);
}

}  // namespace vr
