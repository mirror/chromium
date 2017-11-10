// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_COMMON_URL_UTILS_H_
#define COMPONENTS_AUTOFILL_CONTENT_COMMON_URL_UTILS_H_

#include <url/gurl.h>

namespace autofill {

// Helper function that strips any authentication data, as well as query and
// ref portions of URL
GURL StripAuthAndParams(const GURL& document_url);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_COMMON_URL_UTILS_H_
