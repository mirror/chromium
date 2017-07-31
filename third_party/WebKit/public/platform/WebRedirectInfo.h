// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebRedirectInfo_h
#define WebRedirectInfo_h

#include "WebReferrerPolicy.h"
#include "WebString.h"
#include "WebURL.h"

namespace blink {

class WebRedirectInfo {
 public:
  WebRedirectInfo(const WebString& new_method,
                  const WebURL& new_url,
                  const WebURL& new_first_party_for_cookies,
                  const WebString& new_referrer,
                  WebReferrerPolicy new_referrer_policy)
      : new_method_(new_method),
        new_url_(new_url),
        new_first_party_for_cookies_(new_first_party_for_cookies),
        new_referrer_(new_referrer),
        new_referrer_policy_(new_referrer_policy) {}

  const WebString& new_method() const { return new_method_; }
  const WebURL& new_url() const { return new_url_; }
  const WebURL& new_first_party_for_cookies() const {
    return new_first_party_for_cookies_;
  }
  const WebString& new_referrer() const { return new_referrer_; }
  WebReferrerPolicy new_referrer_policy() const { return new_referrer_policy_; }

 private:
  WebString new_method_;
  WebURL new_url_;
  WebURL new_first_party_for_cookies_;
  WebString new_referrer_;
  WebReferrerPolicy new_referrer_policy_;
};

}  // namespace blink

#endif  // WebRedirectInfo_h
