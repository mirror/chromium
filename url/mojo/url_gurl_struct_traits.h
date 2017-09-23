// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef URL_MOJO_URL_GURL_STRUCT_TRAITS_H_
#define URL_MOJO_URL_GURL_STRUCT_TRAITS_H_

#include "base/strings/string_piece.h"
#include "url/gurl.h"
#include "url/mojo/url.mojom.h"
#include "url/url_constants.h"

namespace mojo {

template <>
struct StructTraits<url::mojom::UrlDataView, GURL> {
  static bool is_valid(const GURL& r) { return r.is_valid(); }

  static base::StringPiece spec(const GURL& r) {
    if (r.possibly_invalid_spec().length() > url::kMaxURLChars ||
        !r.is_valid()) {
      return base::StringPiece();
    }

    return r.possibly_invalid_spec();
  }

  static bool Read(url::mojom::UrlDataView data, GURL* out);
};

}

#endif  // URL_MOJO_URL_GURL_STRUCT_TRAITS_H_
