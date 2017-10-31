// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef URL_EMPTY_STRING_H_
#define URL_EMPTY_STRING_H_

#include <string>

#include "url/url_export.h"

namespace url {
namespace internal {

URL_EXPORT const std::string& EmptyString();

}  // namespace internal
}  // namespace url

#endif  // URL_EMPTY_STRING_H
