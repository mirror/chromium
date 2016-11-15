// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_network_transaction.h"

#include <ostream>
#include <unordered_map>

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_auth_controller.h"
#include "net/http/http_auth_handler_mock.h"
#include "net/http/http_request_info.h"
#include "net/socket/socket_test_util.h"
#include "net/spdy/spdy_test_util_common.h"
#include "net/test/gtest_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

// Preparations for the HttpNetworkTransactionAuthTest test suite.
namespace {

// Scheme used by a proxy or server. See Participant.
enum class Scheme {
  NONE,  // No participant. Only valid when used to describe a proxy and
         // indicates that there's no proxy involved.
  HTTP,  // The target uses http:// scheme.
  HTTPS  // The target uses https:// scheme.
};

// Type of authentication token generation involved when authenticating with the
// participant. See Participant.
enum class TokenGenerationMode {
  NONE,  // None. Indicates that authentication is not required for the target.
  SYNC,  // Participant requires authentication. Token generation
         // completes synchronously.
  ASYNC  // Participant requires authentication. Token generation is
         // asynchronous.
};

// Result of authentication token generation. See Participant.
enum class TokenGenerationResult {
  OK,              // No error.
  IDENTITY_FATAL,  // An error that invalidates the identity used to generate an
                   // auth token.
  SCHEME_FATAL,    // An error that causes the authentication scheme to be
                   // invalidated for the remainder of the network transaction.
  TRANSACTION_FATAL  // An error that causes the network transaction to fail.
};

// Describes a participant in a network transaction for the purpose of
// describing a test case for HTTP authentication. A participant may be a proxy
// or server. A server is always required for a network transaction, but a proxy
// is optional.
struct Participant {
  static Participant None() {
    return Participant{Scheme::NONE, TokenGenerationMode::NONE,
                       TokenGenerationResult::OK};
  }

  static Participant Http() {
    return Participant{Scheme::HTTP, TokenGenerationMode::NONE,
                       TokenGenerationResult::OK};
  }

  static Participant Https() {
    return Participant{Scheme::HTTPS, TokenGenerationMode::NONE,
                       TokenGenerationResult::OK};
  }

  static Participant HttpWithAuth(TokenGenerationResult result) {
    return Participant{Scheme::HTTP, TokenGenerationMode::SYNC, result};
  }

  static Participant HttpsWithAuth(TokenGenerationResult result) {
    return Participant{Scheme::HTTPS, TokenGenerationMode::SYNC, result};
  }

  bool operator==(const Participant& that) const {
    DCHECK(is_valid() && that.is_valid());
    return target_scheme == that.target_scheme &&
           token_generation_mode == that.token_generation_mode &&
           first_token_generation_result == that.first_token_generation_result;
  }

  // Used with STL collections.
  std::size_t Hash() const {
    DCHECK(is_valid());
    return (static_cast<std::size_t>(target_scheme) << 0) |
           (static_cast<std::size_t>(token_generation_mode) << 2) |
           (static_cast<std::size_t>(first_token_generation_result) << 4);
  }

  // Some targets are considered invalid. E.g. if the |target_scheme| is NONE,
  // then |token_generation_mode| must be NONE and
  // |first_token_generation_result| must be OK.
  bool is_valid() const {
    if (target_scheme == Scheme::NONE &&
        token_generation_mode != TokenGenerationMode::NONE)
      return false;
    if (token_generation_mode == TokenGenerationMode::NONE &&
        first_token_generation_result != TokenGenerationResult::OK)
      return false;
    return true;
  }

  // AuthTargets can differ only by whether their token generation is
  // synchronous or not. Such AuthTargets are considered equivalent for the
  // purpose of test case generation since the externally visible behavior is
  // identical (i.e. we can use the same test expectations even though the
  // underlying code paths are different).
  //
  // In order to de-duplicate test cases, we consider the SYNC case to be
  // canonical.
  Participant GetCanonicalParticipant() const {
    DCHECK(is_valid());
    return Participant{target_scheme,
                       token_generation_mode == TokenGenerationMode::ASYNC
                           ? TokenGenerationMode::SYNC
                           : token_generation_mode,
                       first_token_generation_result};
  }

  Scheme target_scheme;
  TokenGenerationMode token_generation_mode;

  // The first authentication error for this target. The test cases assume that
  // the second round always succeed, if there is a second round.
  TokenGenerationResult first_token_generation_result;
};

// Describes the network configuration for a single test case. A single
// configuration describes the two targets -- proxy, and server -- including the
// type of HTTP authentication required by each and the result of attempting to
// authenticate with each respective participant.
struct NetworkConfiguration {
  Participant proxy;
  Participant server;

  bool operator==(const NetworkConfiguration& that) const {
    return proxy == that.proxy && server == that.server;
  }

  // Used with STL collections.
  std::size_t Hash() const { return (proxy.Hash() << 8) | (server.Hash()); }

  // A proxy is optional, but a server is mandatory.
  bool is_valid() const {
    return proxy.is_valid() && server.is_valid() &&
           server.target_scheme != Scheme::NONE;
  }

  // A canonical configuration is one where both participants are themselves
  // canonical.
  NetworkConfiguration GetCanonicalConfiguration() const {
    return NetworkConfiguration{proxy.GetCanonicalParticipant(),
                                server.GetCanonicalParticipant()};
  }
};

// Hash function used to with STL collections. The hash and the companion
// equivalence predicate de-duplicates so that we don't need to store redundant
// configurations.
struct CanonicalNetworkConfigurationHash {
  std::size_t operator()(const NetworkConfiguration& network_config) const {
    return network_config.GetCanonicalConfiguration().Hash();
  }
};

// Equivalence predicate used with STL collections. Companion to
// CanonicalNetworkConfigurationHash.
struct CanonicalNetworkConfigurationEquals {
  bool operator()(const NetworkConfiguration& l,
                  const NetworkConfiguration& r) const {
    return l.GetCanonicalConfiguration() == r.GetCanonicalConfiguration();
  }
};

enum ShouldUseSSL { USE_SSL };

// A single round of the state machine for a network transaction. Multiple of
// these may be required for some network test cases.
//
// The first round is initiated using HttpNetworkTransaction::Start(). Where
// HTTP authentication is the factor that terminates the HttpNetworkTransaction
// state machine, subsequent rounds are initiated using
// HttpNetworkTransaction::RestartWithAuth().
//
// In all constructors:
//
//   |write_arg| : Describes writes to the socket. This is data that's being
//       sent to the server from the client.
//
//   |read_arg| : Describes reads from the socket. This is data that's being
//       read from the server.
//
//   |expected_rv_arg| : The expected return value of calling Start() or
//       RestartWithAuth() after ERR_IO_PENDING has been resolved.
//
//   |should_use_ssl| : Should be set to USE_SSL if an SSL socket is required
//       for that round.
//
//   |extra_write_arg|, |extra_read_arg| : Two "sockets" participate in a
//       network transaction round that establish a tunnel. The first outer
//       socket handles the HTTP CONNECT verb and exists between the UA and the
//       proxy. The inner socket (which typically require a SSL socket) handles
//       the HTTP request sent to the server and exists between the UA and the
//       server.
//
//       For such transactions, |write_arg| and |read_arg| describe the outer
//       (tunnel) socket, while |extra_write_arg| and |extra_read_arg| describe
//       the inner (tunneled) socket.
struct TransactionRound {
  TransactionRound(const MockWrite& write_arg,
                   const MockRead& read_arg,
                   int expected_rv_arg)
      : write(write_arg), read(read_arg), expected_rv(expected_rv_arg) {}

  TransactionRound(const MockWrite& write_arg,
                   const MockRead& read_arg,
                   int expected_rv_arg,
                   ShouldUseSSL should_use_ssl)
      : write(write_arg),
        read(read_arg),
        expected_rv(expected_rv_arg),
        need_ssl_socket(true) {}

  TransactionRound(const MockWrite& write_arg,
                   const MockRead& read_arg,
                   int expected_rv_arg,
                   const MockWrite* extra_write_arg,
                   const MockRead* extra_read_arg)
      : write(write_arg),
        read(read_arg),
        expected_rv(expected_rv_arg),
        extra_write(extra_write_arg),
        extra_read(extra_read_arg) {}

  TransactionRound(const MockWrite& write_arg,
                   const MockRead& read_arg,
                   int expected_rv_arg,
                   ShouldUseSSL should_use_ssl,
                   const MockWrite* extra_write_arg,
                   const MockRead* extra_read_arg)
      : write(write_arg),
        read(read_arg),
        expected_rv(expected_rv_arg),
        extra_write(extra_write_arg),
        extra_read(extra_read_arg),
        need_ssl_socket(true) {}

  MockWrite write;
  MockRead read;
  int expected_rv = ERR_UNEXPECTED;
  const MockWrite* extra_write = nullptr;
  const MockRead* extra_read = nullptr;
  bool need_ssl_socket = false;
};

struct TransactionRounds {
  int line_number;  // Line number where transaction rounds were defined in this
                    // file. It would be pretty impossible to locate a failing
                    // test case without this.

  std::vector<TransactionRound> rounds;
};

class HttpNetworkTransactionAuthTest
    : public ::testing::TestWithParam<NetworkConfiguration> {
 protected:
  static const TransactionRounds& GetRoundsForNetworkConfiguration(
      const NetworkConfiguration& network_config);

  static net::Error GetNetErrorForTokenGenerationResult(
      TokenGenerationResult error);

  static std::unique_ptr<HttpNetworkSession> CreateSession(
      SpdySessionDependencies* session_deps) {
    return SpdySessionDependencies::SpdyCreateSession(session_deps);
  }

  SpdySessionDependencies session_deps_;
};

const base::string16 kFoo(base::ASCIIToUTF16("foo"));
const base::string16 kBar(base::ASCIIToUTF16("bar"));

auto HttpNetworkTransactionAuthTest::GetRoundsForNetworkConfiguration(
    const NetworkConfiguration& network_config) -> const TransactionRounds& {
  static const MockWrite kGet(
      "GET / HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Connection: keep-alive\r\n\r\n");
  static const MockWrite kGetViaProxy(
      "GET http://www.example.com/ HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Proxy-Connection: keep-alive\r\n\r\n");
  static const MockWrite kGetWithAuth(
      "GET / HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Connection: keep-alive\r\n"
      "Authorization: auth_token\r\n\r\n");
  static const MockWrite kGetWithProxyAuth(
      "GET http://www.example.com/ HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Proxy-Connection: keep-alive\r\n"
      "Proxy-Authorization: auth_token\r\n\r\n");
  static const MockWrite kGetWithAuthViaProxy(
      "GET http://www.example.com/ HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Proxy-Connection: keep-alive\r\n"
      "Authorization: auth_token\r\n\r\n");
  static const MockWrite kGetWithAuthAndProxyAuth(
      "GET http://www.example.com/ HTTP/1.1\r\n"
      "Host: www.example.com\r\n"
      "Proxy-Connection: keep-alive\r\n"
      "Proxy-Authorization: auth_token\r\n"
      "Authorization: auth_token\r\n\r\n");
  static const MockWrite kConnect(
      "CONNECT www.example.com:443 HTTP/1.1\r\n"
      "Host: www.example.com:443\r\n"
      "Proxy-Connection: keep-alive\r\n\r\n");
  static const MockWrite kConnectWithProxyAuth(
      "CONNECT www.example.com:443 HTTP/1.1\r\n"
      "Host: www.example.com:443\r\n"
      "Proxy-Connection: keep-alive\r\n"
      "Proxy-Authorization: auth_token\r\n\r\n");

  static const MockRead kSuccessResponse(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html; charset=iso-8859-1\r\n"
      "Content-Length: 3\r\n\r\n"
      "Yes");
  static const MockRead kNotReached("Should not be called.");
  static const MockRead kServerAuthResponse(
      "HTTP/1.1 401 Unauthorized\r\n"
      "WWW-Authenticate: Mock realm=server\r\n"
      "Content-Type: text/html; charset=iso-8859-1\r\n"
      "Content-Length: 14\r\n\r\n"
      "Unauthorized\r\n");
  static const MockRead kProxyAuthResponse(
      "HTTP/1.1 407 Unauthorized\r\n"
      "Proxy-Authenticate: Mock realm=proxy\r\n"
      "Proxy-Connection: close\r\n"
      "Content-Type: text/html; charset=iso-8859-1\r\n"
      "Content-Length: 14\r\n\r\n"
      "Unauthorized\r\n");
  static const MockRead kConnectSucceededResponse(
      "HTTP/1.1 200 Connection Established\r\n\r\n");

  using TestCase = std::pair<NetworkConfiguration, TransactionRounds>;

  // Use of |static| is an attempt at limiting initialization of these
  // containers to once per process.
  static const std::vector<TestCase> kTestCaseVector = {
      // Non-authenticating HTTP server with a direct connection.
      TestCase{{Participant::None(), Participant::Http()},
               {__LINE__, {TransactionRound(kGet, kSuccessResponse, OK)}}},

      // Authenticating HTTP server with a direct connection.
      TestCase{{Participant::None(),
                Participant::HttpWithAuth(TokenGenerationResult::OK)},
               {__LINE__,
                {TransactionRound(kGet, kServerAuthResponse, OK),
                 TransactionRound(kGetWithAuth, kSuccessResponse, OK)}}},

      TestCase{{Participant::None(),
                Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL)},
               {__LINE__,
                {TransactionRound(kGet, kServerAuthResponse, OK),
                 TransactionRound(kGet, kServerAuthResponse, OK)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::OK)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::None(),
           Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kGet, kServerAuthResponse, OK),
            TransactionRound(kGet, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::None(),
           Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kGet, kServerAuthResponse, OK),
            // The second round uses a HttpAuthHandlerMock that always
            // succeeds.
            TransactionRound(kGet, kServerAuthResponse, OK),
            TransactionRound(kGetWithAuth, kSuccessResponse, OK)}}},

      // Non-authenticating HTTP server through a non-authenticating proxy.
      TestCase{
          {Participant::Http(), Participant::Http()},
          {__LINE__, {TransactionRound(kGetViaProxy, kSuccessResponse, OK)}}},

      // Authenticating HTTP server through a non-authenticating proxy.
      TestCase{
          {Participant::Http(),
           Participant::HttpWithAuth(TokenGenerationResult::OK)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kServerAuthResponse, OK),
            TransactionRound(kGetWithAuthViaProxy, kSuccessResponse, OK)}}},

      TestCase{
          {Participant::Http(),
           Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kServerAuthResponse, OK),
            TransactionRound(kGetViaProxy, kServerAuthResponse, OK),
            TransactionRound(kGetWithAuthViaProxy, kSuccessResponse, OK)}}},

      TestCase{
          {Participant::Http(),
           Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kServerAuthResponse, OK),
            TransactionRound(kGetViaProxy, kNotReached, ERR_FAILED)}}},

      // Non-authenticating HTTP server through an authenticating proxy.
      TestCase{{Participant::HttpWithAuth(TokenGenerationResult::OK),
                Participant::Http()},
               {__LINE__,
                {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
                 TransactionRound(kGetWithProxyAuth, kSuccessResponse, OK)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::Http()},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kSuccessResponse, OK)}}},

      // Authenticating HTTP server through an authenticating proxy.
      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::OK),
           Participant::HttpWithAuth(TokenGenerationResult::OK)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithAuthAndProxyAuth, kSuccessResponse, OK)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::OK),
           Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithAuthAndProxyAuth, kSuccessResponse, OK)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::OK)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithAuthAndProxyAuth, kSuccessResponse, OK)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithAuthAndProxyAuth, kSuccessResponse, OK)}}},

      // Non-authenticating HTTPS server with a direct connection.
      TestCase{
          {Participant::None(), Participant::Https()},
          {__LINE__, {TransactionRound(kGet, kSuccessResponse, OK, USE_SSL)}}},

      // Authenticating HTTPS server with a direct connection.
      TestCase{
          {Participant::None(),
           Participant::HttpsWithAuth(TokenGenerationResult::OK)},
          {__LINE__,
           {TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL),
            TransactionRound(kGetWithAuth, kSuccessResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::None(),
           Participant::HttpsWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL),
            TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL),
            TransactionRound(kGetWithAuth, kSuccessResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::None(), Participant::HttpsWithAuth(
                                    TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL),
            TransactionRound(kGet, kNotReached, ERR_FAILED, USE_SSL)}}},

      // Non-authenticating HTTPS server with a non-authenticating proxy.
      TestCase{{Participant::Http(), Participant::Https()},
               {__LINE__,
                {TransactionRound(kConnect, kConnectSucceededResponse, OK,
                                  USE_SSL, &kGet, &kSuccessResponse)}}},

      // Authenticating HTTPS server through a non-authenticating proxy.
      TestCase{
          {Participant::Http(),
           Participant::HttpsWithAuth(TokenGenerationResult::OK)},
          {__LINE__,
           {TransactionRound(kConnect, kConnectSucceededResponse, OK, USE_SSL,
                             &kGet, &kServerAuthResponse),
            TransactionRound(kGetWithAuth, kSuccessResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::Http(),
           Participant::HttpsWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kConnectSucceededResponse, OK, USE_SSL,
                             &kGet, &kServerAuthResponse),
            TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL),
            TransactionRound(kGetWithAuth, kSuccessResponse, OK, USE_SSL)}}},

      // Non-Authenticating HTTPS server through an authenticating proxy.
      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::OK),
           Participant::Https()},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnectWithProxyAuth, kConnectSucceededResponse,
                             OK, USE_SSL, &kGet, &kSuccessResponse)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::Https()},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kConnectSucceededResponse, OK, USE_SSL,
                             &kGet, &kSuccessResponse)}}},

      TestCase{{Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
                Participant::HttpsWithAuth(TokenGenerationResult::OK)},
               {__LINE__,
                {TransactionRound(kConnect, kProxyAuthResponse, OK),
                 TransactionRound(kConnect, kConnectSucceededResponse, OK,
                                  USE_SSL, &kGet, &kSuccessResponse)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::HttpsWithAuth(TokenGenerationResult::OK)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kNotReached, ERR_FAILED)}}},

      // Authenticating HTTPS server through an authenticating proxy.
      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::OK),
           Participant::HttpsWithAuth(TokenGenerationResult::OK)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnectWithProxyAuth, kConnectSucceededResponse,
                             OK, USE_SSL, &kGet, &kServerAuthResponse),
            TransactionRound(kGetWithAuth, kSuccessResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::OK),
           Participant::HttpsWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnectWithProxyAuth, kConnectSucceededResponse,
                             OK, USE_SSL, &kGet, &kServerAuthResponse),
            TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL),
            TransactionRound(kGetWithAuth, kSuccessResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::HttpsWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnectWithProxyAuth, kConnectSucceededResponse,
                             OK, USE_SSL, &kGet, &kServerAuthResponse),
            TransactionRound(kGet, kSuccessResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::HttpsWithAuth(
               TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnectWithProxyAuth, kConnectSucceededResponse,
                             OK, USE_SSL, &kGet, &kServerAuthResponse),
            TransactionRound(kGet, kNotReached, ERR_FAILED, USE_SSL)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::OK),
           Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::OK),
           Participant::HttpsWithAuth(TokenGenerationResult::SCHEME_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK, USE_SSL),
            TransactionRound(kConnectWithProxyAuth, kConnectSucceededResponse,
                             OK, USE_SSL, &kGet, &kServerAuthResponse),
            TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::OK),
           Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::OK),
           Participant::HttpsWithAuth(
               TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK, USE_SSL),
            TransactionRound(kConnectWithProxyAuth, kConnectSucceededResponse,
                             OK, USE_SSL, &kGet, &kServerAuthResponse),
            TransactionRound(kGet, kNotReached, ERR_FAILED, USE_SSL)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::Http()},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::Https()},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::HttpsWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::HttpsWithAuth(TokenGenerationResult::SCHEME_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kNotReached, ERR_FAILED)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL),
           Participant::HttpsWithAuth(
               TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kNotReached, ERR_FAILED)}}},

      TestCase{{Participant::Http(),
                Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL)},
               {__LINE__,
                {TransactionRound(kGetViaProxy, kServerAuthResponse, OK),
                 TransactionRound(kGetViaProxy, kServerAuthResponse, OK)}}},

      TestCase{{Participant::Http(), Participant::HttpsWithAuth(
                                         TokenGenerationResult::SCHEME_FATAL)},
               {__LINE__,
                {TransactionRound(kConnect, kConnectSucceededResponse, OK,
                                  USE_SSL, &kGet, &kServerAuthResponse),
                 TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::Http(), Participant::HttpsWithAuth(
                                    TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kConnectSucceededResponse, OK, USE_SSL,
                             &kGet, &kServerAuthResponse),
            TransactionRound(kGet, kNotReached, ERR_FAILED, USE_SSL)}}},

      TestCase{{Participant::None(), Participant::HttpsWithAuth(
                                         TokenGenerationResult::SCHEME_FATAL)},
               {__LINE__,
                {TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL),
                 TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::HttpsWithAuth(TokenGenerationResult::OK)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnectWithProxyAuth, kConnectSucceededResponse,
                             OK, USE_SSL, &kGet, &kServerAuthResponse),
            TransactionRound(kGetWithAuth, kSuccessResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::HttpsWithAuth(TokenGenerationResult::SCHEME_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnectWithProxyAuth, kConnectSucceededResponse,
                             OK, USE_SSL, &kGet, &kServerAuthResponse),
            TransactionRound(kGet, kServerAuthResponse, OK, USE_SSL)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kServerAuthResponse, OK),
            TransactionRound(kGetWithProxyAuth, kNotReached, ERR_FAILED)}}},

      TestCase{{Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
                Participant::Http()},
               {__LINE__,
                {
                    TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
                    TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
                }}},

      TestCase{{Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
                Participant::Https()},
               {__LINE__,
                {TransactionRound(kConnect, kProxyAuthResponse, OK),
                 TransactionRound(kConnect, kProxyAuthResponse,
                                  ERR_PROXY_AUTH_UNSUPPORTED)}}},

      TestCase{{Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
                Participant::HttpWithAuth(TokenGenerationResult::OK)},
               {__LINE__,
                {
                    TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
                    TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
                }}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {
               TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
               TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
           }}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
           Participant::HttpsWithAuth(TokenGenerationResult::IDENTITY_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kProxyAuthResponse,
                             ERR_PROXY_AUTH_UNSUPPORTED)}}},

      TestCase{{Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
                Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL)},
               {__LINE__,
                {
                    TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
                    TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
                }}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
           Participant::HttpsWithAuth(TokenGenerationResult::SCHEME_FATAL)},
          {__LINE__,
           {TransactionRound(kConnect, kProxyAuthResponse, OK),
            TransactionRound(kConnect, kProxyAuthResponse,
                             ERR_PROXY_AUTH_UNSUPPORTED)}}},

      TestCase{
          {Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
           Participant::HttpWithAuth(TokenGenerationResult::TRANSACTION_FATAL)},
          {__LINE__,
           {
               TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
               TransactionRound(kGetViaProxy, kProxyAuthResponse, OK),
           }}},

      TestCase{{Participant::HttpWithAuth(TokenGenerationResult::SCHEME_FATAL),
                Participant::HttpsWithAuth(
                    TokenGenerationResult::TRANSACTION_FATAL)},
               {__LINE__,
                {TransactionRound(kConnect, kProxyAuthResponse, OK),
                 TransactionRound(kConnect, kProxyAuthResponse,
                                  ERR_PROXY_AUTH_UNSUPPORTED)}}},
  };

  // Using the canonical hash and comparator during construction of the map
  // ensures that we don't have unnecessary duplication of test cases here.
  static const std::unordered_map<NetworkConfiguration, TransactionRounds,
                                  CanonicalNetworkConfigurationHash,
                                  CanonicalNetworkConfigurationEquals>
      kTestCaseMap(kTestCaseVector.begin(), kTestCaseVector.end());

  static const TransactionRounds kDummyRounds = {0};

  EXPECT_EQ(kTestCaseMap.size(), kTestCaseVector.size())
      << "That last test case you added was probably a duplicate of something.";
  auto result = kTestCaseMap.find(network_config);
  return result == kTestCaseMap.end() ? kDummyRounds : result->second;
}

std::ostream& operator<<(std::ostream& os, const Scheme& t) {
  switch (t) {
    case Scheme::NONE:
      os << "Scheme::NONE";
      break;
    case Scheme::HTTP:
      os << "Scheme::HTTP";
      break;
    case Scheme::HTTPS:
      os << "Scheme::HTTPS";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const TokenGenerationMode& t) {
  switch (t) {
    case TokenGenerationMode::NONE:
      os << "TokenGenerationMode::NONE";
      break;
    case TokenGenerationMode::SYNC:
      os << "TokenGenerationMode::SYNC";
      break;
    case TokenGenerationMode::ASYNC:
      os << "TokenGenerationMode::ASYNC";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const TokenGenerationResult& e) {
  switch (e) {
    case TokenGenerationResult::OK:
      os << "TokenGenerationResult::OK";
      break;
    case TokenGenerationResult::IDENTITY_FATAL:
      os << "TokenGenerationResult::IDENTITY_FATAL";
      break;
    case TokenGenerationResult::SCHEME_FATAL:
      os << "TokenGenerationResult::SCHEME_FATAL";
      break;
    case TokenGenerationResult::TRANSACTION_FATAL:
      os << "TokenGenerationResult::TRANSACTION_FATAL";
      break;
  }
  return os;
}

// Use representative errors from each failure mode.
net::Error HttpNetworkTransactionAuthTest::GetNetErrorForTokenGenerationResult(
    TokenGenerationResult error) {
  switch (error) {
    case TokenGenerationResult::OK:
      return OK;
    case TokenGenerationResult::IDENTITY_FATAL:
      return ERR_INVALID_AUTH_CREDENTIALS;
    case TokenGenerationResult::SCHEME_FATAL:
      return ERR_UNSUPPORTED_AUTH_SCHEME;
    case TokenGenerationResult::TRANSACTION_FATAL:
      return ERR_FAILED;
  }
  NOTREACHED();
  return ERR_FAILED;
}

std::vector<NetworkConfiguration> GenerateNetworkConfigurationPermutations() {
  std::vector<NetworkConfiguration> permutations;

  const Scheme kProxyTargets[] = {Scheme::NONE, Scheme::HTTP};
  const Scheme kServerTargets[] = {Scheme::HTTP, Scheme::HTTPS};
  const TokenGenerationResult kAuthErrors[] = {
      TokenGenerationResult::OK, TokenGenerationResult::IDENTITY_FATAL,
      TokenGenerationResult::SCHEME_FATAL,
      TokenGenerationResult::TRANSACTION_FATAL};
  const TokenGenerationMode kAuthTimings[] = {TokenGenerationMode::NONE,
                                              TokenGenerationMode::SYNC,
                                              TokenGenerationMode::ASYNC};

  for (auto proxy_target : kProxyTargets)
    for (auto proxy_auth_timing : kAuthTimings)
      for (auto first_proxy_auth_error : kAuthErrors)
        for (auto server_target : kServerTargets)
          for (auto server_auth_timing : kAuthTimings)
            for (auto first_server_auth_error : kAuthErrors) {
              Participant proxy = {proxy_target, proxy_auth_timing,
                                   first_proxy_auth_error};
              Participant server = {server_target, server_auth_timing,
                                    first_server_auth_error};
              NetworkConfiguration p = {proxy, server};

              if (p.is_valid())
                permutations.push_back(p);
            }

  return permutations;
}

INSTANTIATE_TEST_CASE_P(
    _,
    HttpNetworkTransactionAuthTest,
    ::testing::ValuesIn(GenerateNetworkConfigurationPermutations()));

}  // namespace

TEST_P(HttpNetworkTransactionAuthTest, _) {
  static const char kServer[] = "http://www.example.com";
  static const char kSecureServer[] = "https://www.example.com";
  static const char kProxy[] = "myproxy:70";

  const auto& network_config = GetParam();
  const auto& test_rounds = GetRoundsForNetworkConfiguration(network_config);

  SCOPED_TRACE(::testing::Message()
               << "Test case: " << std::endl
               << "{{" << network_config.proxy.target_scheme << ","
               << network_config.proxy.token_generation_mode << ","
               << network_config.proxy.first_token_generation_result << "},"
               << std::endl
               << "{" << network_config.server.target_scheme << ","
               << network_config.server.token_generation_mode << ","
               << network_config.server.first_token_generation_result << "}}");
  SCOPED_TRACE(::testing::Message()
               << "Test rounds defined at " << test_rounds.line_number);
  ASSERT_THAT(test_rounds.rounds.size(), ::testing::Gt(0u));

  HttpAuthHandlerMock::Factory* auth_factory(
      new HttpAuthHandlerMock::Factory());
  session_deps_.http_auth_handler_factory.reset(auth_factory);
  SSLInfo empty_ssl_info;

  ASSERT_THAT(network_config.proxy.target_scheme,
              ::testing::AnyOf(Scheme::NONE, Scheme::HTTP));
  if (network_config.proxy.token_generation_mode != TokenGenerationMode::NONE) {
    // Currently on HTTP proxies are tested. There is no material difference
    // between authenticating with an HTTP proxy vs an HTTPS proxy.
    ASSERT_EQ(Scheme::HTTP, network_config.proxy.target_scheme);
    for (unsigned n = 0; n < test_rounds.rounds.size(); n++) {
      HttpAuthHandlerMock* auth_handler(new HttpAuthHandlerMock());
      std::string auth_challenge = "Mock realm=proxy";
      GURL origin(kProxy);
      HttpAuthChallengeTokenizer tokenizer(auth_challenge.begin(),
                                           auth_challenge.end());
      auth_handler->InitFromChallenge(&tokenizer, HttpAuth::AUTH_PROXY,
                                      empty_ssl_info, origin,
                                      NetLogWithSource());
      auth_handler->SetGenerateExpectation(
          network_config.proxy.token_generation_mode ==
              TokenGenerationMode::ASYNC,
          n == 0 ? GetNetErrorForTokenGenerationResult(
                       network_config.proxy.first_token_generation_result)
                 : OK);
      auth_factory->AddMockHandler(auth_handler, HttpAuth::AUTH_PROXY);
    }
  }

  ASSERT_THAT(network_config.server.target_scheme,
              ::testing::AnyOf(Scheme::HTTP, Scheme::HTTPS));
  if (network_config.server.token_generation_mode !=
      TokenGenerationMode::NONE) {
    HttpAuthHandlerMock* auth_handler(new HttpAuthHandlerMock());
    std::string auth_challenge = "Mock realm=server";
    GURL origin(network_config.server.target_scheme == Scheme::HTTP
                    ? kServer
                    : kSecureServer);
    HttpAuthChallengeTokenizer tokenizer(auth_challenge.begin(),
                                         auth_challenge.end());
    auth_handler->InitFromChallenge(&tokenizer, HttpAuth::AUTH_SERVER,
                                    empty_ssl_info, origin, NetLogWithSource());
    auth_handler->SetGenerateExpectation(
        network_config.server.token_generation_mode ==
            TokenGenerationMode::ASYNC,
        GetNetErrorForTokenGenerationResult(
            network_config.server.first_token_generation_result));
    auth_factory->AddMockHandler(auth_handler, HttpAuth::AUTH_SERVER);

    // The second handler always succeeds. It should only be used where there
    // are multiple auth sessions for server auth in the same network
    // transaction using the same auth scheme.
    std::unique_ptr<HttpAuthHandlerMock> second_handler =
        base::MakeUnique<HttpAuthHandlerMock>();
    second_handler->InitFromChallenge(&tokenizer, HttpAuth::AUTH_SERVER,
                                      empty_ssl_info, origin,
                                      NetLogWithSource());
    second_handler->SetGenerateExpectation(true, OK);
    auth_factory->AddMockHandler(second_handler.release(),
                                 HttpAuth::AUTH_SERVER);
  }
  if (network_config.proxy.target_scheme == Scheme::HTTP) {
    session_deps_.proxy_service = ProxyService::CreateFixed(kProxy);
  } else {
    session_deps_.proxy_service = ProxyService::CreateDirect();
  }

  HttpRequestInfo request;
  request.method = "GET";
  request.url =
      GURL(network_config.server.target_scheme == Scheme::HTTPS ? kSecureServer
                                                                : kServer);

  std::unique_ptr<HttpNetworkSession> session(CreateSession(&session_deps_));

  SSLSocketDataProvider ssl_socket_data_provider(SYNCHRONOUS, OK);

  std::vector<std::vector<MockRead>> mock_reads(1);
  std::vector<std::vector<MockWrite>> mock_writes(1);
  for (const auto& read_write_round : test_rounds.rounds) {
    // Set up expected reads and writes.
    mock_reads.back().push_back(read_write_round.read);
    mock_writes.back().push_back(read_write_round.write);

    // kProxyAuthResponse uses Proxy-Connection: close which means that the
    // socket is closed and a new one will be created for the next request.
    if (base::StringPiece(read_write_round.read.data,
                          read_write_round.read.data_len)
            .find("Proxy-Connection: close") != base::StringPiece::npos) {
      mock_reads.push_back(std::vector<MockRead>());
      mock_writes.push_back(std::vector<MockWrite>());
    }

    if (read_write_round.extra_read)
      mock_reads.back().push_back(*read_write_round.extra_read);

    if (read_write_round.extra_write)
      mock_writes.back().push_back(*read_write_round.extra_write);

    if (read_write_round.need_ssl_socket)
      session_deps_.socket_factory->AddSSLSocketDataProvider(
          &ssl_socket_data_provider);
  }

  std::vector<std::unique_ptr<StaticSocketDataProvider>> data_providers;
  for (size_t i = 0; i < mock_reads.size(); ++i) {
    data_providers.push_back(base::WrapUnique(new StaticSocketDataProvider(
        mock_reads[i].data(), mock_reads[i].size(), mock_writes[i].data(),
        mock_writes[i].size())));
    session_deps_.socket_factory->AddSocketDataProvider(
        data_providers.back().get());
  }

  // Transaction must be created after DataProviders, so it's destroyed before
  // them.
  HttpNetworkTransaction transaction(DEFAULT_PRIORITY, session.get());
  TestCompletionCallback callback;
  bool first_round = true;
  int rv = OK;

  for (const auto& read_write_round : test_rounds.rounds) {
    SCOPED_TRACE(
        ::testing::Message()
        << "Round "
        << std::distance(&test_rounds.rounds.front(), &read_write_round));

    ASSERT_THAT(rv, test::IsOk());

    if (first_round) {
      rv = transaction.Start(&request, callback.callback(), NetLogWithSource());
      first_round = false;
    } else {
      const HttpResponseInfo* response = transaction.GetResponseInfo();
      ASSERT_TRUE(response);
      ASSERT_TRUE(response->auth_challenge);

      rv = transaction.RestartWithAuth(AuthCredentials(kFoo, kBar),
                                       callback.callback());
    }

    if (rv == ERR_IO_PENDING)
      rv = callback.WaitForResult();

    EXPECT_THAT(rv, test::IsError(read_write_round.expected_rv));
  }

  const HttpResponseInfo* response = transaction.GetResponseInfo();
  ASSERT_TRUE(response);
  EXPECT_FALSE(response->auth_challenge);
  EXPECT_FALSE(transaction.IsReadyToRestartForAuth());
}

}  // namespace net
