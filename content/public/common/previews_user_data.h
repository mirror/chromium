// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_PREVIEWS_USER_DATA_H_
#define CONTENT_PUBLIC_COMMON_PREVIEWS_USER_DATA_H_

#include "base/strings/string16.h"
#include "base/supports_user_data.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT PreviewsUserData : public base::SupportsUserData::Data {
 public:
  static const void* const kUserDataKey;

  PreviewsUserData(base::string16 placeholder_text);
  ~PreviewsUserData() override;

  const base::string16& placeholder_text() const { return placeholder_text_; }

 private:
  base::string16 placeholder_text_;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_PREVIEWS_USER_DATA_H_
