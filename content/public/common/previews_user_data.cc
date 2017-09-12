// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/previews_user_data.h"

#include <utility>

namespace content {

// static
const void* const PreviewsUserData::kUserDataKey =
    &PreviewsUserData::kUserDataKey;

PreviewsUserData::PreviewsUserData(base::string16 placeholder_text)
    : placeholder_text_(std::move(placeholder_text)) {}

PreviewsUserData::~PreviewsUserData() {}

}  // namespace content
