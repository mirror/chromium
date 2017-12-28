// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/referrer_struct_traits.h"

#include "url/mojo/url_gurl_struct_traits.h"

namespace mojo {

// static
::blink::WebReferrerPolicy
StructTraits<::blink::mojom::ReferrerDataView, content::Referrer>::policy(
    const content::Referrer& r) {
  return content::Referrer::NetReferrerPolicyToBlinkReferrerPolicy(r.policy);
}

// static
bool StructTraits<::blink::mojom::ReferrerDataView, content::Referrer>::Read(
    ::blink::mojom::ReferrerDataView data,
    content::Referrer* out) {
  blink::WebReferrerPolicy policy;
  if (!data.ReadUrl(&out->url) || !data.ReadPolicy(&policy))
    return false;

  out->policy = content::Referrer::ReferrerPolicyForUrlRequest(policy);
  return true;
}

}  // namespace mojo
