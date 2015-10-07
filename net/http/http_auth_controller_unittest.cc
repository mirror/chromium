// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_controller.h"

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_auth.h"
#include "net/http/http_auth_cache.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_auth_filter.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_handler_mock.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_util.h"
#include "net/http/mock_allow_url_security_manager.h"
#include "net/log/net_log.h"
#include "net/ssl/ssl_info.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::ValuesIn;

namespace net {

namespace {

enum HandlerRunMode {
  RUN_HANDLER_SYNC,
  RUN_HANDLER_ASYNC
};

enum SchemeState {
  SCHEME_IS_DISABLED,
  SCHEME_IS_ENABLED
};

scoped_refptr<HttpResponseHeaders> HeadersFromString(const char* string) {
  std::string raw_string(string);
  std::string headers_string = HttpUtil::AssembleRawHeaders(
      raw_string.c_str(), raw_string.length());
  scoped_refptr<HttpResponseHeaders> headers(
      new HttpResponseHeaders(headers_string));
  return headers;
}

// Runs an HttpAuthController with a single round mock auth handler
// that returns |handler_rv| on token generation.  The handler runs in
// async if |run_mode| is RUN_HANDLER_ASYNC.  Upon completion, the
// return value of the controller is tested against
// |expected_controller_rv|.  |scheme_state| indicates whether the
// auth scheme used should be disabled after this run.
void RunSingleRoundAuthTest(HandlerRunMode run_mode,
                            int handler_rv,
                            int expected_controller_rv,
                            SchemeState scheme_state) {
  SCOPED_TRACE(::testing::Message()
               << "run_mode:" << run_mode << " handler_rv:" << handler_rv
               << " expected_controller_rv:" << expected_controller_rv
               << " scheme_state:" << scheme_state);
  BoundNetLog dummy_log;
  HttpAuthCache dummy_auth_cache;

  HttpRequestInfo request;
  request.method = "GET";
  request.url = GURL("http://example.com");

  HttpResponseInfo response;
  response.headers = HeadersFromString(
      "HTTP/1.1 407\r\n"
      "Proxy-Authenticate: MOCK foo\r\n"
      "\r\n");

  HttpAuthHandlerMock::Factory auth_handler_factory;
  scoped_ptr<HttpAuthHandlerMock> auth_handler(new HttpAuthHandlerMock());
  auth_handler->SetGenerateExpectation((run_mode == RUN_HANDLER_ASYNC),
                                       handler_rv);
  auth_handler_factory.AddMockHandler(auth_handler.Pass(),
                                      HttpAuthHandlerCreateReason::CHALLENGE);

  scoped_refptr<HttpAuthController> controller(
      new HttpAuthController(HttpAuth::AUTH_PROXY,
                             GURL("http://example.com"),
                             &dummy_auth_cache, &auth_handler_factory));
  SSLInfo null_ssl_info;
  ASSERT_EQ(OK, controller->HandleAuthChallenge(headers, null_ssl_info, false,
                                                false, dummy_log));
  ASSERT_TRUE(controller->HaveAuthHandler());
  EXPECT_FALSE(controller->HaveAuth());
  controller->ResetAuth(
      AuthCredentials(base::ASCIIToUTF16("a"), base::ASCIIToUTF16("b")));
  EXPECT_TRUE(controller->HaveAuth());

  TestCompletionCallback callback;
  result = controller->MaybeGenerateAuthToken(&request, callback.callback(),
                                              dummy_log);
  if (run_mode == RUN_HANDLER_ASYNC)
    ASSERT_EQ(ERR_IO_PENDING, result);
  EXPECT_EQ(expected_controller_rv, callback.GetResult(result));
  EXPECT_EQ((scheme_state == SCHEME_IS_DISABLED),
            controller->IsAuthSchemeDisabled("mock"));
}

}  // namespace

// If an HttpAuthHandler returns an error code that indicates a
// permanent error, the HttpAuthController should disable the scheme
// used and retry the request.
TEST(HttpAuthControllerTest, PermanentErrors_Sync) {
  RunSingleRoundAuthTest(RUN_HANDLER_SYNC,
                         ERR_UNEXPECTED_SECURITY_LIBRARY_STATUS,
                         OK, SCHEME_IS_DISABLED);
}

// Similar to the above test. Now try an async handler that returns
// ERR_MISSING_AUTH_CREDENTIALS. Async and sync handlers invoke different code
// paths in HttpAuthController when generating tokens.
TEST(HttpAuthControllerTest, PermanentErrors_Async) {
  RunSingleRoundAuthTest(RUN_HANDLER_ASYNC, ERR_MISSING_AUTH_CREDENTIALS, OK,
                         SCHEME_IS_DISABLED);
}

// If a non-permanent error is returned by the handler, then the controller
// should leave the scheme enabled.
TEST(HttpAuthControllerTest, NonPermanentErrors_Sync) {
  RunSingleRoundAuthTest(RUN_HANDLER_SYNC, ERR_INVALID_AUTH_CREDENTIALS,
                         ERR_INVALID_AUTH_CREDENTIALS, SCHEME_IS_ENABLED);
}

// If a non-permanent error is returned by the handler, then the controller
// should leave the scheme enabled.
TEST(HttpAuthControllerTest, NonPermanentErrors_Async) {
  RunSingleRoundAuthTest(RUN_HANDLER_ASYNC, ERR_INVALID_AUTH_CREDENTIALS,
                         ERR_INVALID_AUTH_CREDENTIALS, SCHEME_IS_ENABLED);
}

// If an HttpAuthHandler indicates that it doesn't allow explicit
// credentials, don't prompt for credentials.
TEST(HttpAuthControllerTest, NoExplicitCredentialsAllowed) {
  BoundNetLog dummy_log;
  HttpAuthCache dummy_auth_cache;
  HttpRequestInfo request;
  request.method = "GET";
  request.url = GURL("http://example.com");

  HttpRequestHeaders request_headers;
  HttpResponseInfo response;
  response.headers = HeadersFromString(
      "HTTP/1.1 401\r\n"
      "WWW-Authenticate: Ernie\r\n"
      "WWW-Authenticate: Bert\r\n"
      "\r\n");

  HttpAuthHandlerMock::Factory auth_handler_factory;

  // Handler for the first attempt at authentication. "Ernie" handler accepts
  // the default identity and successfully constructs a token.  Handler for the
  scoped_ptr<HttpAuthHandlerMock> auth_handler(new HttpAuthHandlerMock());
  auth_handler->set_allows_default_credentials(true);
  auth_handler->set_allows_explicit_credentials(false);
  auth_handler->set_expected_auth_scheme("ernie");
  auth_handler_factory.AddMockHandler(auth_handler.Pass(),
                                      HttpAuthHandlerCreateReason::CHALLENGE);

  scoped_refptr<HttpAuthController> controller(
      new HttpAuthController(HttpAuth::AUTH_SERVER,
                             GURL("http://example.com"),
                             &dummy_auth_cache, &auth_handler_factory));
  SSLInfo null_ssl_info;
  ASSERT_EQ(OK, controller->HandleAuthChallenge(headers, null_ssl_info, false,
                                                false, dummy_log));
  TestCompletionCallback completion_calback;
  int rv = controller->HandleAuthChallenge(
      response, completion_calback.callback(), dummy_log);
  ASSERT_EQ(OK, completion_calback.GetResult(rv));
  ASSERT_TRUE(controller->HaveAuthHandler());
  EXPECT_TRUE(controller->HaveAuth());
  EXPECT_FALSE(auth_handler_factory.HaveAuthHandlers());

  // Should only succeed if we are using the "Ernie" MockHandler.
  ASSERT_EQ(OK, controller->MaybeGenerateAuthToken(
                    &request, completion_calback.callback(), dummy_log));
  controller->AddAuthorizationHeader(&request_headers);

  // Handlers for the second attempt. Neither should be used to generate a
  // token. Instead the controller should realize that there are no viable
  // identities to use with the "Ernie" handler and fail.
  auth_handler.reset(new HttpAuthHandlerMock());
  auth_handler->set_allows_default_credentials(true);
  auth_handler->set_allows_explicit_credentials(false);
  auth_handler->set_expected_auth_scheme("ernie");
  auth_handler_factory.AddMockHandler(auth_handler.Pass(),
                                      HttpAuthHandlerCreateReason::CHALLENGE);

  // Fallback handlers for the second attempt. The "Ernie" handler should be
  // discarded due to the disabled scheme, and the "Bert" handler should
  // successfully be used to generate a token.
  auth_handler.reset(new HttpAuthHandlerMock());
  auth_handler->set_allows_default_credentials(false);
  auth_handler->set_allows_explicit_credentials(true);
  auth_handler->set_expected_auth_scheme("bert");
  auth_handler_factory.AddMockHandler(auth_handler.Pass(),
                                      HttpAuthHandlerCreateReason::CHALLENGE);

  // Once a token is generated, simulate the receipt of a server response
  // indicating that the authentication attempt was rejected.
  ASSERT_EQ(OK, controller->HandleAuthChallenge(headers, null_ssl_info, false,
                                                false, dummy_log));
  TestCompletionCallback completion_calback_rejected;
  rv = controller->HandleAuthChallenge(
      response, completion_calback_rejected.callback(), dummy_log);
  ASSERT_EQ(OK, completion_calback_rejected.GetResult(rv));
  ASSERT_TRUE(controller->HaveAuthHandler());
  EXPECT_FALSE(controller->HaveAuth());
  controller->ResetAuth(AuthCredentials(base::ASCIIToUTF16("Hello"),
                        base::string16()));
  EXPECT_TRUE(controller->HaveAuth());
  EXPECT_TRUE(controller->IsAuthSchemeDisabled("ernie"));
  EXPECT_FALSE(controller->IsAuthSchemeDisabled("bert"));

  // Should only succeed if we are using the "Bert" MockHandler.
  EXPECT_EQ(OK, controller->MaybeGenerateAuthToken(
                    &request, completion_calback.callback(), dummy_log));
}

namespace {

struct ChallengeResponse {
  const char* const response_headers = nullptr;
  const char* const authorization_header = nullptr;

  ChallengeResponse(const char* const response_headers,
                    const char* const authorization_header)
      : response_headers(response_headers),
        authorization_header(authorization_header) {}
};

struct MockAuthHandler {
  std::string scheme;
  int init_rv = OK;
  int generate_rv = OK;

  MockAuthHandler(const std::string& scheme, int int_rv, int generate_rv)
      : scheme(scheme), init_rv(int_rv), generate_rv(generate_rv) {}
};

struct ChallengeResponseTestCase {
  std::vector<ChallengeResponse> rounds;
  std::vector<MockAuthHandler> handlers;
  bool run_handler_async = false;

  ChallengeResponseTestCase& WithChallengeResponse(
      const char* response_headers,
      const char* authorization_header) {
    rounds.push_back(ChallengeResponse(response_headers, authorization_header));
    return *this;
  }

  ChallengeResponseTestCase& WithHandler(const base::StringPiece& scheme,
                                         int init_rv,
                                         int generate_rv) {
    handlers.push_back(
        MockAuthHandler(scheme.as_string(), init_rv, generate_rv));
    return *this;
  }
};

std::vector<ChallengeResponseTestCase> SyncAndAsync(
    const ChallengeResponseTestCase& test_case) {
  std::vector<ChallengeResponseTestCase> test_cases;
  ChallengeResponseTestCase test_case_copy = test_case;

  test_case_copy.run_handler_async = false;
  test_cases.push_back(test_case_copy);

  test_case_copy.run_handler_async = true;
  test_cases.push_back(test_case_copy);
  return test_cases;
}

class HttpAuthControllerTest
    : public ::testing::TestWithParam<ChallengeResponseTestCase> {};

}  // namespace

TEST_P(HttpAuthControllerTest, Run) {
  const ChallengeResponseTestCase& params = GetParam();
  BoundNetLog dummy_log;
  HttpAuthCache dummy_auth_cache;
  GURL url("http://example.com");
  HttpAuthHandlerMock::Factory auth_factory;

  for (const auto& handler : params.handlers) {
    scoped_ptr<HttpAuthHandlerMock> mock_handler(new HttpAuthHandlerMock);
    mock_handler->set_expected_auth_scheme(handler.scheme);
    mock_handler->set_expect_multiple_challenges(handler.scheme == "ntlm" ||
                                                 handler.scheme == "negotiate");
    mock_handler->SetInitExpectation(params.run_handler_async, handler.init_rv);
    mock_handler->SetGenerateExpectation(params.run_handler_async,
                                         handler.generate_rv);
    auth_factory.AddMockHandler(mock_handler.Pass(),
                                HttpAuthHandlerCreateReason::CHALLENGE);
  }

  HttpRequestInfo request_info;

  scoped_refptr<HttpAuthController> auth_controller(new HttpAuthController(
      HttpAuth::AUTH_SERVER, url, &dummy_auth_cache, &auth_factory));
  for (const auto& round : params.rounds) {
    if (!round.response_headers)
      break;
    HttpResponseInfo response_info;
    response_info.headers = HeadersFromString(round.response_headers);
    TestCompletionCallback callback;

    int result = auth_controller->HandleAuthChallenge(
        response_info, callback.callback(), dummy_log);
    EXPECT_EQ(OK, callback.GetResult(result));

    if (!auth_controller->HaveAuthHandler()) {
      EXPECT_STREQ("", round.authorization_header);
      continue;
    }

    if (!auth_controller->HaveAuth()) {
      auth_controller->ResetAuth(
          AuthCredentials(base::ASCIIToUTF16("a"), base::ASCIIToUTF16("b")));
    }

    result = auth_controller->MaybeGenerateAuthToken(
        &request_info, callback.callback(), dummy_log);
    EXPECT_EQ(OK, callback.GetResult(result));
    HttpRequestHeaders headers;
    auth_controller->AddAuthorizationHeader(&headers);
    std::string authorization_header;
    headers.GetHeader(
        HttpAuth::GetAuthorizationHeaderName(HttpAuth::AUTH_SERVER),
        &authorization_header);
    EXPECT_STREQ(round.authorization_header, authorization_header.c_str());
  }

  EXPECT_FALSE(auth_factory.HaveAuthHandlers());
}

// Picks Basic because that's the only supported scheme.
INSTANTIATE_TEST_CASE_P(
    PickSupportedScheme,
    HttpAuthControllerTest,
    ValuesIn(
        SyncAndAsync(ChallengeResponseTestCase()
                         .WithChallengeResponse(
                             "HTTP/1.1 401\n"
                             "Y: Digest realm=\"X\", nonce=\"aaaaaaaaaa\"\n"
                             "www-authenticate: Basic realm=\"BasicRealm\"\n",
                             "basic auth_token,realm=\"BasicRealm\"")
                         .WithHandler("basic", OK, OK))));

// No supported schemes.
INSTANTIATE_TEST_CASE_P(
    NoSupportedSchemes,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(ChallengeResponseTestCase().WithChallengeResponse(
        "HTTP/1.1 401\n"
        "Y: Digest realm=\"FooBar\", nonce=\"aaaaaaaaaa\"\n"
        "www-authenticate: Fake realm=\"FooBar\"\n",
        ""))));

// Pick Digest over Basic.
INSTANTIATE_TEST_CASE_P(
    DigestOverBasic,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse(
                "HTTP/1.1 401\n"
                "www-authenticate: Basic realm=\"FooBar\"\n"
                "www-authenticate: Fake realm=\"FooBar\"\n"
                "www-authenticate: nonce=\"aaaaaaaaaa\"\n"
                "www-authenticate: Digest realm=\"DigestRealm\", "
                "nonce=\"aaaaaaaaaa\"\n",
                "digest auth_token,realm=\"DigestRealm\", nonce=\"aaaaaaaaaa\"")
            .WithHandler("digest", OK, OK))));

// Handle an empty header correctly.
INSTANTIATE_TEST_CASE_P(
    EmptyHeader,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(ChallengeResponseTestCase().WithChallengeResponse(
        "HTTP/1.1 401\n"
        "Y: Digest realm=\"X\", nonce=\"aaaaaaaaaa\"\n"
        "www-authenticate:\n",
        ""))));

// NTLM appears earlier in the list, Negotiate is preferred.
INSTANTIATE_TEST_CASE_P(
    NegotiateOverNTLM,
    HttpAuthControllerTest,
    ValuesIn(
        SyncAndAsync(ChallengeResponseTestCase()
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: NTLM\n"
                                                "WWW-Authenticate: Negotiate\n",
                                                "negotiate auth_token")
                         .WithHandler("negotiate", OK, OK))));

// Two rounds. Basic isn't connection oriented and will use up a new handler
// for the second challenge.
INSTANTIATE_TEST_CASE_P(
    SecondChallengeForBasic,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic first_round\n",
                                   "basic auth_token,first_round")
            .WithHandler("basic", OK, OK)
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic second_round\n",
                                   "basic auth_token,second_round")
            .WithHandler("basic", OK, OK))));

// Correctly deal with a missing challenge for the second round.
INSTANTIATE_TEST_CASE_P(
    SecondChallengeIsMissing,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic first_round\n",
                                   "basic auth_token,first_round")
            .WithHandler("basic", OK, OK)
            .WithChallengeResponse("HTTP/1.1 401\n", ""))));

// Correctly deal with an empty challenge for the second round.
INSTANTIATE_TEST_CASE_P(
    SecondChallengeIsEmpty,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic first_round\n",
                                   "basic auth_token,first_round")
            .WithHandler("basic", OK, OK)
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate:\n",
                                   ""))));

// The second round introduces a preferred scheme.
INSTANTIATE_TEST_CASE_P(
    SecondChallengeHasPreferredScheme,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic first_round\n",
                                   "basic auth_token,first_round")
            .WithHandler("basic", OK, OK)
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic second_round\n"
                                   "WWW-Authenticate: Negotiate\n",
                                   "negotiate auth_token")
            .WithHandler("negotiate", OK, OK))));

// If there are two challenges, then the auth controller should pick the first
// one.
INSTANTIATE_TEST_CASE_P(
    MultipleChallengesForSameScheme,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic first_round\n",
                                   "basic auth_token,first_round")
            .WithHandler("basic", OK, OK)
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic second_round_1\n"
                                   "WWW-Authenticate: Basic second_round_2\n",
                                   "basic auth_token,second_round_1")
            .WithHandler("basic", OK, OK))));

// As above, but the handler will reject the first challenge of the second
// round. The auth controller should use the second challenge instead.
INSTANTIATE_TEST_CASE_P(
    RejectChallengeForTheSameSchemeInSingleResponse,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic first_round\n",
                                   "basic auth_token,first_round")
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic second_round_1\n"
                                   "WWW-Authenticate: Basic second_round_2\n",
                                   "basic auth_token,second_round_2")
            .WithHandler("basic", OK, OK)
            .WithHandler("basic", ERR_INVALID_AUTH_CREDENTIALS, OK)
            .WithHandler("basic", OK, OK))));

// Connection based schemes will treat new auth challenges for the same scheme
// as acceptance and continuance of the current handshake if there's a
// parameter.
INSTANTIATE_TEST_CASE_P(
    MultiRoundAuth_Continuations,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Negotiate\n",
                                   "negotiate auth_token")
            .WithHandler("negotiate", OK, OK)
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Negotiate foo\n",
                                   "negotiate continuation,foo"))));

// As above, but picks the first challenge for the second round.
INSTANTIATE_TEST_CASE_P(
    MultiRoundAuth_ContinuationsWithMultipleChallenges,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Negotiate\n",
                                   "negotiate auth_token")
            .WithHandler("negotiate", OK, OK)
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Negotiate foo\n"
                                   "WWW-Authenticate: Negotiate\n",
                                   "negotiate continuation,foo"))));

// As above. The first challenge in the second round counts as a rejection, so
// the controller should create a new handler and restart the handshake.
INSTANTIATE_TEST_CASE_P(
    MultiRoundAuth_ContinuationsWithMultipleChallengesAndRejection,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Negotiate\n",
                                   "negotiate auth_token")
            .WithHandler("negotiate", OK, OK)
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Negotiate\n"
                                   "WWW-Authenticate: Negotiate continuation\n",
                                   "negotiate auth_token")
            .WithHandler("negotiate", OK, OK))));

// Multiround authentication with rejection. The rejection comes in the form of
// an empty challenge for the same scheme.
INSTANTIATE_TEST_CASE_P(
    MultiRoundAuth_RejectionViaEmptyChallenge,
    HttpAuthControllerTest,
    ValuesIn(
        SyncAndAsync(ChallengeResponseTestCase()
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: Negotiate\n",
                                                "negotiate auth_token")
                         .WithHandler("negotiate", OK, OK)
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: Negotiate\n",
                                                "negotiate auth_token")
                         .WithHandler("negotiate", OK, OK)
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: Negotiate\n",
                                                "negotiate auth_token")
                         .WithHandler("negotiate", OK, OK))));

// Multiround authentication with rejection. The rejection comes in the form of
// a response with no challenge for the original scheme.
INSTANTIATE_TEST_CASE_P(
    MultiRoundAuth_RejectionViaSchemeChange,
    HttpAuthControllerTest,
    ValuesIn(
        SyncAndAsync(ChallengeResponseTestCase()
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: Negotiate\n",
                                                "negotiate auth_token")
                         .WithHandler("negotiate", OK, OK)
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: Basic foo\n",
                                                "basic auth_token,foo")
                         .WithHandler("basic", OK, OK))));

// Dealing with empty and missing second round challenges for connection
// oriented schemes.
INSTANTIATE_TEST_CASE_P(
    MultiRoundAuth_EmptySecondChallenge,
    HttpAuthControllerTest,
    ValuesIn(
        SyncAndAsync(ChallengeResponseTestCase()
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: Negotiate\n",
                                                "negotiate auth_token")
                         .WithHandler("negotiate", OK, OK)
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate:\n",
                                                ""))));
INSTANTIATE_TEST_CASE_P(
    MultiRoundAuth_MissingSecondChallenge,
    HttpAuthControllerTest,
    ValuesIn(
        SyncAndAsync(ChallengeResponseTestCase()
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: Negotiate\n",
                                                "negotiate auth_token")
                         .WithHandler("negotiate", OK, OK)
                         .WithChallengeResponse("HTTP/1.1 401\n", ""))));

// Should pick matching challenge even if other challenges are present.
INSTANTIATE_TEST_CASE_P(
    MultiRoundAuth_PicksCorrectChallenge,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Negotiate\n",
                                   "negotiate auth_token")
            .WithHandler("negotiate", OK, OK)
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic foo\n"
                                   "WWW-Authenticate: Negotiate bar\n",
                                   "negotiate continuation,bar"))));
INSTANTIATE_TEST_CASE_P(
    MultiRoundAuth_PicksCorrectChallengeWithPreferredChallenge,
    HttpAuthControllerTest,
    ValuesIn(
        SyncAndAsync(ChallengeResponseTestCase()
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: NTLM\n",
                                                "ntlm auth_token")
                         .WithHandler("ntlm", OK, OK)
                         .WithChallengeResponse("HTTP/1.1 401\n"
                                                "WWW-Authenticate: Negotiate\n"
                                                "WWW-Authenticate: NTLM bar\n",
                                                "ntlm continuation,bar"))));

// If an auth handler fails to initialize, the controller should try the next
// suitable challenge.
INSTANTIATE_TEST_CASE_P(
    FallThroughInvalidHeaders,
    HttpAuthControllerTest,
    ValuesIn(SyncAndAsync(
        ChallengeResponseTestCase()
            .WithChallengeResponse("HTTP/1.1 401\n"
                                   "WWW-Authenticate: Basic abc;\n"
                                   "WWW-Authenticate: Digest abc;\n"
                                   "WWW-Authenticate: Basic valid",
                                   "basic auth_token,valid")
            // The first challenge chosen is Digest. But the handler fails.
            .WithHandler("digest", ERR_INVALID_AUTH_CREDENTIALS, OK)
            // Next the controller tries the first Basic challenge, which also
            // fails.
            .WithHandler("basic", ERR_INVALID_AUTH_CREDENTIALS, OK)
            // Finally, the second Basic challenge succeeds.
            .WithHandler("basic", OK, OK))));

// Identities are used in the correct order.
TEST(HttpAuthControllerTest, IdentityPriorityOrder) {}

// Identities from abstract sources should be available to a new handler if the
// server changes authentication scheme or if all identity sources were
// exhausted for a handler.
TEST(HttpAuthControllerTest, IdentityReuseAcrossHandlers) {}

}  // namespace net
