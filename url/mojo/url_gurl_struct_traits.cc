// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url/mojo/url_gurl_struct_traits.h"

namespace mojo {

bool StructTraits<url::mojom::UrlDataView, GURL>::Read(
    url::mojom::UrlDataView data,
    GURL* out) {
  bool is_valid = data.is_valid();
  if (!is_valid) {
    *out = GURL();
    return true;
  }

  base::StringPiece spec_string;
  if (!data.ReadSpec(&spec_string))
    return false;

  if (spec_string.length() > url::kMaxURLChars)
    return false;

  *out = GURL(spec_string);
  if (!spec_string.empty() && !out->is_valid())
    return false;

  return true;
}

}  // namespace mojo
