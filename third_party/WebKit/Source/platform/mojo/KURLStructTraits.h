// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef KURLStructTraits_h
#define KURLStructTraits_h

#include "platform/PlatformExport.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/text/WTFString.h"
#include "url/mojo/url.mojom-blink.h"
#include "url/url_constants.h"

namespace mojo {

template <>
struct PLATFORM_EXPORT
    StructTraits<url::mojom::blink::Url::DataView, ::blink::KURL> {
  static bool is_valid(const ::blink::KURL& url) { return url.IsValid(); }
  static const WTF::String& spec(const ::blink::KURL& url) {
    if (!url.IsValid() || url.GetString().length() > url::kMaxURLChars) {
      return g_empty_string;
    }

    return url.GetString();
  }

  static bool Read(url::mojom::blink::Url::DataView, ::blink::KURL* out);
};
}

#endif  // KURLStructTraits_h
