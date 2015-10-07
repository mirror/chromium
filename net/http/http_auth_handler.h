// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_HANDLER_H_
#define NET_HTTP_HTTP_AUTH_HANDLER_H_

#include <string>

#include "net/base/completion_callback.h"
#include "net/base/net_export.h"
#include "net/http/http_auth.h"
#include "net/http/http_auth_cache.h"
#include "net/log/net_log.h"

namespace net {

class HttpAuthChallengeTokenizer;
struct HttpRequestInfo;
class SSLInfo;

// HttpAuthHandler is the interface for the authentication schemes
// (basic, digest, NTLM, Negotiate). Each authentication scheme is expected to
// subclass HttpAuthHandler.
//
// The lifecycle of a HttpAuthHandler -- i.e. how to use HttpAuthHandler --:
//
// 1. Construct via HttpAuthHandlerFactory::CreateAuthHandlerForScheme() or
//    HttpAuthHandlerFactory::CreateAndInitPreemptiveAuthHandler().
//
//    Upon creation, an HttpAuthHandler is not associated with any HTTP
//    connection or session. That association is established by calling
//    HandleInitialChallenge().
//
// 2. Call HttpAuthHandler::HandleInitialChallenge(). This *must* be the first
//    method or getter invoked by an owner of HttpAuthHandler. The invocation
//    establishes the state for HttpAuthHandler associating it with an
//    authentication target, an origin and an initial challenge from the server
//    which may establish scheme specific state.
//
//    If the return value of HandleInitialChallenge() indicates failure, then
//    this handler can no longer be used. The owner is expected to discard the
//    handler without invoking any other methods.
//
// 3. Repeat the following steps:
//
//    3.1. Call GenerateAuthToken() to generate an authentication token. The
//      token returned by this method call should be passed via a suitable
//      authorization header to the server. E.g. "Authorization: <returned
//      token>".
//
//    3.2 If the server responds with an additional challenge, call
//      HandleAnotherChallenge(). This method continues an authentication
//      handshake. The AuthorizationResult return value indicates whether the
//      handler can continue with the current authentication handshake or if it
//      has encountered an error. A return value of AUTHORIZATION_RESULT_ACCEPT
//      indicates that the handler can continue. In all other cases, the handler
//      should be considered no longer usable.
//
class NET_EXPORT_PRIVATE HttpAuthHandler {
 public:
  virtual ~HttpAuthHandler();

  int InitializeFromCacheEntry(HttpAuthCache::Entry* cache_entry,
                               HttpAuth::Target target,
                               const BoundNetLog& net_log);

  // Initializes the handler and associates it with the specified |target| and
  // |origin|. The |net_log| parameter indicates BoundNetLog to be used for the
  // lifetime of this handler. |challenge| is required and *must* match the
  // authentication scheme of this handler.
  //
  // Returns a Error value. The HttpAuthHandler can only be used if the return
  // value is OK.
  //
  // Note: This method *must* be the first method to be invoked on the
  // HttpAuthHandler.
  int HandleInitialChallenge(const HttpAuthChallengeTokenizer& challenge,
                             const HttpResponseInfo& response_info,
                             HttpAuth::Target target,
                         const SSLInfo& ssl_info,
                             const GURL& origin,
                             const BoundNetLog& net_log,
                             const CompletionCallback& callback);

  // Generates an authentication token, potentially asynchronously.
  //
  // If NeedsIdentity() is true, then the value of |credentials| indicates how
  // the authentication identity is established.
  //
  // 1. If |credentials| is nullptr, then the handler attempts to use ambient
  //    credentials to establish an identity. Passing in a nullptr for
  //    |credentials| is only valid if AllowsDefaultCredentials() returns true.
  //
  // 2. If |credentials| is is not nullptr, then it will be used to establish
  //    the authentication identity. Passing in a non-null |credentials| is only
  //    valid if AllowsExplicitCredentials() returns true.
  //
  // |request| is required and should represent the request that will, if the
  // call is successful, contain the generated authentication token.
  //
  // The return value is a net error code.
  //
  // If |OK| is returned, |*auth_token| is filled in with an authentication
  // token which can be sent via an appropriate authorization header. E.g.
  // "Authorization: <returned token>".
  //
  // If |ERR_IO_PENDING| is returned, |*auth_token| will be filled in
  // asynchronously and |callback| will be invoked. The lifetime of
  // |request|, |callback|, and |auth_token| must last until |callback| is
  // invoked, but |credentials| is only used during the initial call.
  //
  // All other return codes indicate that there was a problem generating a
  // token, and the value of |*auth_token| is unspecified.
  int GenerateAuthToken(const AuthCredentials* credentials,
                        const HttpRequestInfo& request,
                        const CompletionCallback& callback,
                        std::string* auth_token);

  // Determines how the previous authorization attempt was received.
  //
  // This is called when the server/proxy responds with a 401/407 after an
  // earlier authorization attempt. Although this normally means that the
  // previous attempt was rejected, in multi-round schemes such as
  // NTLM+Negotiate it may indicate that another round of challenge+response
  // is required. For Digest authentication it may also mean that the previous
  // attempt used a stale nonce (and nonce-count) and that a new attempt should
  // be made with a different nonce provided in the challenge.
  //
  // |challenge| must be non-NULL and have already tokenized the
  // authentication scheme, but none of the tokens occurring after the
  // authentication scheme.
  virtual HttpAuth::AuthorizationResult HandleAnotherChallenge(
      const HttpAuthChallengeTokenizer& challenge) = 0;

  // The authentication scheme as an enumerated value.
  const std::string& auth_scheme() const { return auth_scheme_; }

  // The realm, encoded as UTF-8. This may be empty. Only valid after
  // HandleInitialChallenge() is called.
  const std::string& realm() const {
    return realm_;
  }

  // The challenge which was issued when creating the handler. Only valid after
  // HandleInitialChallenge() is called.
  const std::string& challenge() const { return auth_challenge_; }

  // Authentication target. Only valid after HandleInitialChallenge() is called.
  HttpAuth::Target target() const {
    return target_;
  }

  // Returns the proxy or server which issued the authentication challenge
  // that this HttpAuthHandler is handling. The URL includes scheme, host, and
  // port, but does not include path. Only valid after HandleInitialChallenge()
  // is called.
  const GURL& origin() const {
    return origin_;
  }

  // Returns true if the response to the current authentication challenge
  // requires an identity. This function can be called after a successful call
  // to HandleInitialChallenge() or HandleAnotherChallenge() to determine
  // whether the next GenerateAuthToken() call should specify an identity.
  //
  // TODO(wtc): Find a better way to handle a multi-round challenge-response
  // sequence used by a connection-based authentication scheme.
  virtual bool NeedsIdentity();

  // Returns whether the default credentials may be used for the |origin| passed
  // into |HandleInitialChallenge|. If true, the user does not need to be
  // prompted for username and password to establish credentials.  NOTE: SSO is
  // a potential security risk.
  // TODO(cbentzel): Add a pointer to Firefox documentation about risk.
  virtual bool AllowsDefaultCredentials();

  // Returns whether explicit credentials can be used with this handler. If true
  // the credentials passed in to GenerateAuthToken() may specify explicit
  // credentials.
  virtual bool AllowsExplicitCredentials();

 protected:
  // |scheme| sets the return value for auth_scheme().
  HttpAuthHandler(const std::string& scheme);

  // Initializes the handler using a challenge issued by a server.  |challenge|
  // must be non-NULL and have already tokenized the authentication scheme, but
  // none of the tokens occurring after the authentication scheme.
  // Implementations are expected to initialize the following members: scheme_,
  // realm_
  virtual int InitializeFromChallengeInternal(
      const HttpAuthChallengeTokenizer& challenge,
      const HttpResponseInfo& response_with_challenge,
      const CompletionCallback& callback) = 0;

  // TODO(asanka): Update comment
  virtual int InitializeFromCacheEntryInternal(
      HttpAuthCache::Entry* cache_entry) = 0;

  // |GenerateAuthTokenImpl()} is the auth-scheme specific implementation
  // of generating the next auth token. Callers should use |GenerateAuthToken()|
  // which will in turn call |GenerateAuthTokenImpl()|
  virtual int GenerateAuthTokenImpl(const AuthCredentials* credentials,
                                    const HttpRequestInfo& request,
                                    const CompletionCallback& callback,
                                    std::string* auth_token) = 0;

  // The auth-scheme as a lowercase ASCII RFC 2616 2.2 token.
  std::string auth_scheme_;

  // The realm, encoded as UTF-8. Used by "basic" and "digest".
  std::string realm_;

  // The auth challenge.
  std::string auth_challenge_;

  // The {scheme, host, port} for the authentication target.  Used by "ntlm"
  // and "negotiate" to construct the service principal name.
  GURL origin_;

  // Whether this authentication request is for a proxy server, or an
  // origin server.
  HttpAuth::Target target_;

  BoundNetLog net_log_;

 private:
  void OnGenerateAuthTokenComplete(int rv);
  void FinishGenerateAuthToken();

  CompletionCallback callback_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_HANDLER_H_
