// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PERMAFILL_WELL_KNOWN_INTERCEPTOR_H_
#define CONTENT_BROWSER_PERMAFILL_WELL_KNOWN_INTERCEPTOR_H_

namespace net {

class NetworkDelegate;
class URLRequest;
class URLRequestJob;

}  // namespace net

namespace permafill {

net::URLRequestJob* InterceptWellKnownURL(net::URLRequest*,
                                          net::NetworkDelegate*);

}  // namespace permafill

#endif  // CONTENT_BROWSER_PERMAFILL_WELL_KNOWN_INTERCEPTOR_H_
