// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "string_resource.h"

#include "ui/base/l10n/l10n_util.h"

#include "base/strings/utf_string_conversions.h"

namespace vr {

base::string16 GetStringResource(int string_id) {
#if 0
  if (string_id == 6836 || string_id == 6835 || string_id == 6834) {
    LOG(INFO) << "Stubbing string " << string_id;
    return base::UTF8ToUTF16("Test string");
  }
#endif

  //return l10n_util::GetStringUTF16(string_id);
  auto str = l10n_util::GetStringUTF16(string_id);
  LOG(INFO) << "String ID " << string_id << ": '" << str << "'";
  return str;
}

}  // namespace vr
