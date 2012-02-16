// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_CLIENT_CERTIFICATE_SELECTOR_H_
#define CHROME_BROWSER_SSL_CLIENT_CERTIFICATE_SELECTOR_H_
#pragma once

#include "base/callback_forward.h"

class TabContentsWrapper;

namespace net {
class HttpNetworkSession;
class SSLCertRequestInfo;
class X509Certificate;
}

namespace browser {

// Opens a constrained SSL client certificate selection dialog under |parent|,
// offering certificates from |cert_request_info|. When the user has made a
// selection, the dialog will report back to |callback|. |callback| is notified
// when the dialog closes in call cases; if the user cancels the dialog, we call
// with a NULL certificate.
void ShowSSLClientCertificateSelector(
    TabContentsWrapper* wrapper,
    const net::HttpNetworkSession* network_session,
    net::SSLCertRequestInfo* cert_request_info,
    const base::Callback<void(net::X509Certificate*)>& callback);

// Same as above, but doesn't check the UseMoreWebUI flag.  It just calls the
// native implementation.  This lets us have both the WebUI implementation and
// the native implementation linked in at the same time and switchable behind
// a run-time flag.
void ShowNativeSSLClientCertificateSelector(
    TabContentsWrapper* wrapper,
    const net::HttpNetworkSession* network_session,
    net::SSLCertRequestInfo* cert_request_info,
    const base::Callback<void(net::X509Certificate*)>& callback);

}  // namespace browser

#endif  // CHROME_BROWSER_SSL_CLIENT_CERTIFICATE_SELECTOR_H_
