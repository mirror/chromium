// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_H_
#define NET_HTTP_HTTP_AUTH_H_

#include <memory>
#include <set>
#include <string>

#include "net/base/auth.h"
#include "net/base/net_export.h"
#include "net/http/http_util.h"

namespace net {

class BoundNetLog;
class HttpAuthHandler;
class HttpAuthHandlerFactory;
class HttpAuthSchemeSet;
class HttpResponseHeaders;
class SSLInfo;

// Utility class for http authentication.
class NET_EXPORT_PRIVATE HttpAuth {
 public:
  // Http authentication can be done the the proxy server, origin server,
  // or both. This enum tracks who the target is.
  enum Target {
    AUTH_NONE = -1,
    // We depend on the valid targets (!= AUTH_NONE) being usable as indexes
    // in an array, so start from 0.
    AUTH_PROXY = 0,
    AUTH_SERVER = 1,
    AUTH_NUM_TARGETS = 2,
  };

  // What the HTTP WWW-Authenticate/Proxy-Authenticate headers indicate about
  // the previous authorization attempt.
  enum AuthorizationResult {
    AUTHORIZATION_RESULT_ACCEPT,   // The authorization attempt was accepted,
                                   // although there still may be additional
                                   // rounds of challenges.

    AUTHORIZATION_RESULT_REJECT,   // The authorization attempt was rejected.

    AUTHORIZATION_RESULT_STALE,    // (Digest) The nonce used in the
                                   // authorization attempt is stale, but
                                   // otherwise the attempt was valid.

    AUTHORIZATION_RESULT_INVALID,  // The authentication challenge headers are
                                   // poorly formed (the authorization attempt
                                   // itself may have been fine).

    AUTHORIZATION_RESULT_DIFFERENT_REALM,  // The authorization
                                           // attempt was rejected,
                                           // but the realm associated
                                           // with the new challenge
                                           // is different from the
                                           // previous attempt.
  };

  // Describes where the identity used for authentication came from.
  enum IdentitySource {
    // Came from nowhere -- the identity is not initialized.
    IDENT_SRC_NONE,

    // The identity came from the auth cache, by doing a path-based
    // lookup (premptive authorization).
    IDENT_SRC_PATH_LOOKUP,

    // The identity was extracted from a URL of the form:
    // http://<username>:<password>@host:port
    IDENT_SRC_URL,

    // The identity was retrieved from the auth cache, by doing a
    // realm lookup.
    IDENT_SRC_REALM_LOOKUP,

    // The identity was provided by RestartWithAuth -- it likely
    // came from a prompt (or maybe the password manager).
    IDENT_SRC_EXTERNAL,

    // The identity used the default credentials for the computer,
    // on schemes that support single sign-on.
    IDENT_SRC_DEFAULT_CREDENTIALS,
  };

  // Helper structure used by HttpNetworkTransaction to track the current
  // identity being used for authorization.
  struct Identity {
    Identity();
    ~Identity();

    IdentitySource source;
    bool invalid;  // TODO(asanka): Invert this.
    AuthCredentials credentials;
  };

  // Get the name of the header containing the auth challenge (either
  // WWW-Authenticate or Proxy-Authenticate).
  static std::string GetChallengeHeaderName(Target target);

  // Get the name of the header where the credentials go
  // (either Authorization or Proxy-Authorization).
  static std::string GetAuthorizationHeaderName(Target target);

  // Returns a string representation of a Target value that can be used in log
  // messages.
  static std::string GetAuthTargetString(Target target);

  // RFC 7235 states that an authentication scheme is a case insensitive token.
  // This function checks whether |scheme| is a token AND is lowercase.
  static bool IsValidNormalizedScheme(const std::string& scheme);
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_H_
