// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/run_loop.h"
#include "base/test/gtest_util.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "content/browser/webauth/collected_client_data.h"
#include "content/public/browser/render_frame_host.h"
#include "content/test/test_render_frame_host.h"
#include "device/u2f/fake_hid_impl_for_testing.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

using ::testing::_;

using webauth::mojom::AuthenticatorPtr;
using webauth::mojom::AuthenticatorStatus;
using webauth::mojom::MakePublicKeyCredentialOptions;
using webauth::mojom::MakePublicKeyCredentialOptionsPtr;
using webauth::mojom::PublicKeyCredentialRpEntity;
using webauth::mojom::PublicKeyCredentialRpEntityPtr;
using webauth::mojom::PublicKeyCredentialUserEntity;
using webauth::mojom::PublicKeyCredentialUserEntityPtr;
using webauth::mojom::MakeCredentialAuthenticatorResponsePtr;
using webauth::mojom::GetAssertionAuthenticatorResponsePtr;
using webauth::mojom::PublicKeyCredentialParameters;
using webauth::mojom::PublicKeyCredentialParametersPtr;
using webauth::mojom::PublicKeyCredentialRequestOptions;
using webauth::mojom::PublicKeyCredentialRequestOptionsPtr;

namespace {

typedef struct {
  const GURL origin;
  const std::string relying_party_id;
} SecurityTestCases;

constexpr char kTestOrigin1[] = "https://a.google.com";
constexpr char kTestRelyingPartyId[] = "google.com";

// Test data. CBOR test data can be built using the given diagnostic strings
// and the utility at "http://cbor.me/".
constexpr int32_t kCoseEs256 = -7;

constexpr uint8_t kTestChallengeBytes[] = {
    0x68, 0x71, 0x34, 0x96, 0x82, 0x22, 0xEC, 0x17, 0x20, 0x2E, 0x42,
    0x50, 0x5F, 0x8E, 0xD2, 0xB1, 0x6A, 0xE2, 0x2F, 0x16, 0xBB, 0x05,
    0xB8, 0x8C, 0x25, 0xDB, 0x9E, 0x60, 0x26, 0x45, 0xF1, 0x41};

constexpr char kTestRegisterClientDataJsonString[] =
    R"({"challenge":"aHE0loIi7BcgLkJQX47SsWriLxa7BbiMJdueYCZF8UE",)"
    R"("hashAlgorithm":"SHA-256","origin":"google.com","tokenBinding":)"
    R"("unused","type":"webauthn.create"})";

constexpr char kTestSignClientDataJsonString[] =
    R"({"challenge":"aHE0loIi7BcgLkJQX47SsWriLxa7BbiMJdueYCZF8UE",)"
    R"("hashAlgorithm":"SHA-256","origin":"google.com","tokenBinding":)"
    R"("unused","type":"webauthn.get"})";

const SecurityTestCases kValidTestCases[] = {
    {GURL("https://myawesomedomain"), std::string("myawesomedomain")},
    {GURL("https://localhost"), std::string("localhost")},
    {GURL("https://foo.bar.google.com"), std::string("foo.bar.google.com")},
    {GURL("https://foo.bar.google.com"), std::string("bar.google.com")},
    {GURL("https://foo.bar.google.com"), std::string("google.com")},
    {GURL("https://earth.login.awesomecompany"),
     std::string("login.awesomecompany")},
    {GURL("https://google.com:1337"), std::string("google.com")},

    // Hosts with trailing dot valid for rpIds with or without trailing dot.
    // Hosts without trailing dots only matches rpIDs without trailing dot.
    {GURL("https://google.com."), std::string("google.com")},
    {GURL("https://google.com."), std::string("google.com.")},
    {GURL("https://.google.com"), std::string("google.com")},
    {GURL("https://..google.com"), std::string("google.com")},
    {GURL("https://.google.com"), std::string(".google.com")},
    {GURL("https://..google.com"), std::string(".google.com")},
    {GURL("https://google.com.."), std::string("google.com..")},
    {GURL("https://accounts.google.com"), std::string(".google.com")},
};

const SecurityTestCases kCrashingTestCases[] = {
    {GURL("https://google.com"), std::string("com")},
    {GURL("https://login.awesomecompany"), std::string("awesomecompany")},
    {GURL("http://google.com"), std::string("google.com")},
    {GURL("http://myawesomedomain"), std::string("myawesomedomain")},
    {GURL("https://google.com"), std::string("foo.bar.google.com")},
    {GURL("http://myawesomedomain"), std::string("randomdomain")},
    {GURL("https://myawesomedomain"), std::string("randomdomain")},
    {GURL("https://notgoogle.com"), std::string("google.com)")},
    {GURL("https://not-google.com"), std::string("google.com)")},
    {GURL("https://evil.appspot.com"), std::string("appspot.com")},
    {GURL("https://evil.co.uk"), std::string("co.uk")},

    {GURL("https://google.com"), std::string("google.com.")},
    {GURL("https://google.com"), std::string("google.com..")},
    {GURL("https://google.com.."), std::string("google.com")},
    {GURL("https://google.com"), std::string(".google.com")},

    {GURL("https://1.2.3"), std::string("1.2.3")},
    {GURL("https://1.2.3"), std::string("2.3")},

    {GURL("https://127.0.0.1"), std::string("127.0.0.1")},
    {GURL("https://127.0.0.1"), std::string("27.0.0.1")},
    {GURL("https://127.0.0.1"), std::string(".0.0.1")},
    {GURL("https://127.0.0.1"), std::string("0.0.1")},

    {GURL("https://[::127.0.0.1]"), std::string("127.0.0.1")},
    {GURL("https://[::127.0.0.1]"), std::string("[127.0.0.1]")},

    {GURL("https://[::1]"), std::string("1")},
    {GURL("https://[::1]"), std::string("1]")},
    {GURL("https://[::1]"), std::string("::1")},
    {GURL("https://[::1]"), std::string("[::1]")},
    {GURL("https://[1::1]"), std::string("::1")},
    {GURL("https://[1::1]"), std::string("::1]")},
    {GURL("https://[1::1]"), std::string("[::1]")},

    {GURL("http://google.com:443"), std::string("google.com")},
    {GURL("data:google.com"), std::string("google.com")},
    {GURL("data:text/html,google.com"), std::string("google.com")},
    {GURL("ws://google.com"), std::string("google.com")},
    {GURL("wss:///google.com"), std::string("google.com")},
    {GURL("gopher://google.com"), std::string("google.com")},
    {GURL("ftp://google.com"), std::string("google.com")},
    {GURL("file:///google.com"), std::string("google.com")},

    {GURL("data:,"), std::string("")},
    {GURL("https://google.com"), std::string("")},
    {GURL("ws:///google.com"), std::string("")},
    {GURL("wss:///google.com"), std::string("")},
    {GURL("gopher://google.com"), std::string("")},
    {GURL("ftp://google.com"), std::string("")},
    {GURL("file:///google.com"), std::string("")},
};

std::vector<uint8_t> GetTestChallengeBytes() {
  return std::vector<uint8_t>(std::begin(kTestChallengeBytes),
                              std::end(kTestChallengeBytes));
}

PublicKeyCredentialRpEntityPtr GetTestPublicKeyCredentialRPEntity() {
  auto entity = PublicKeyCredentialRpEntity::New();
  entity->id = std::string(kTestRelyingPartyId);
  entity->name = "TestRP@example.com";
  return entity;
}

PublicKeyCredentialUserEntityPtr GetTestPublicKeyCredentialUserEntity() {
  auto entity = PublicKeyCredentialUserEntity::New();
  entity->display_name = "User A. Name";
  std::vector<uint8_t> id(32, 0x0A);
  entity->id = id;
  entity->name = "username@example.com";
  entity->icon = GURL("fakeurl2.png");
  return entity;
}

std::vector<PublicKeyCredentialParametersPtr>
GetTestPublicKeyCredentialParameters(int32_t algorithm_identifier) {
  std::vector<PublicKeyCredentialParametersPtr> parameters;
  auto fake_parameter = PublicKeyCredentialParameters::New();
  fake_parameter->type = webauth::mojom::PublicKeyCredentialType::PUBLIC_KEY;
  fake_parameter->algorithm_identifier = algorithm_identifier;
  parameters.push_back(std::move(fake_parameter));
  return parameters;
}

MakePublicKeyCredentialOptionsPtr GetTestMakePublicKeyCredentialOptions() {
  auto options = MakePublicKeyCredentialOptions::New();
  options->relying_party = GetTestPublicKeyCredentialRPEntity();
  options->user = GetTestPublicKeyCredentialUserEntity();
  options->public_key_parameters =
      GetTestPublicKeyCredentialParameters(kCoseEs256);
  options->challenge.assign(32, 0x0A);
  options->adjusted_timeout = base::TimeDelta::FromMinutes(1);
  return options;
}

PublicKeyCredentialRequestOptionsPtr
GetTestPublicKeyCredentialRequestOptions() {
  auto options = PublicKeyCredentialRequestOptions::New();
  options->relying_party_id = std::string(kTestRelyingPartyId);
  options->challenge.assign(32, 0x0A);
  options->adjusted_timeout = base::TimeDelta::FromMinutes(1);
  return options;
}

CollectedClientData GetTestClientData(std::string type) {
  return CollectedClientData::Create(std::move(type), kTestRelyingPartyId,
                                     GetTestChallengeBytes());
}

class AuthenticatorImplTest : public content::RenderViewHostTestHarness {
 public:
  AuthenticatorImplTest() {}
  ~AuthenticatorImplTest() override {}

 protected:
  // Simulates navigating to a page and getting the page contents and language
  // for that navigation.
  void SimulateNavigation(const GURL& url) {
    if (main_rfh()->GetLastCommittedURL() != url)
      NavigateAndCommit(url);
  }

  AuthenticatorPtr ConnectToAuthenticator() {
    authenticator_impl_ = std::make_unique<AuthenticatorImpl>(main_rfh());
    AuthenticatorPtr authenticator;
    authenticator_impl_->Bind(mojo::MakeRequest(&authenticator));
    return authenticator;
  }

  AuthenticatorPtr ConnectToAuthenticator(
      service_manager::Connector* connector,
      std::unique_ptr<base::OneShotTimer> timer) {
    authenticator_impl_.reset(
        new AuthenticatorImpl(main_rfh(), connector, std::move(timer)));
    AuthenticatorPtr authenticator;
    authenticator_impl_->Bind(mojo::MakeRequest(&authenticator));
    return authenticator;
  }

 private:
  std::unique_ptr<AuthenticatorImpl> authenticator_impl_;
};

class TestMakeCredentialCallback {
 public:
  TestMakeCredentialCallback()
      : callback_(base::BindOnce(&TestMakeCredentialCallback::ReceivedCallback,
                                 base::Unretained(this))) {}
  ~TestMakeCredentialCallback() {}

  void ReceivedCallback(AuthenticatorStatus status,
                        MakeCredentialAuthenticatorResponsePtr credential) {
    response_ = std::make_pair(status, std::move(credential));
    std::move(closure_).Run();
  }

  // TODO(crbug.com/799044) - simplify the runloop usage.
  void WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
  }

  AuthenticatorImpl::MakeCredentialCallback callback() {
    return std::move(callback_);
  }

  AuthenticatorStatus GetResponseStatus() { return response_.first; }

 private:
  std::pair<AuthenticatorStatus, MakeCredentialAuthenticatorResponsePtr>
      response_;
  base::Closure closure_;
  AuthenticatorImpl::MakeCredentialCallback callback_;
  base::RunLoop run_loop_;
};

class TestGetAssertionCallback {
 public:
  TestGetAssertionCallback()
      : callback_(base::BindOnce(&TestGetAssertionCallback::ReceivedCallback,
                                 base::Unretained(this))) {}
  ~TestGetAssertionCallback() {}

  void ReceivedCallback(AuthenticatorStatus status,
                        GetAssertionAuthenticatorResponsePtr credential) {
    response_ = std::make_pair(status, std::move(credential));
    std::move(closure_).Run();
  }

  // TODO(crbug.com/799044) - simplify the runloop usage.
  void WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
  }

  AuthenticatorStatus GetResponseStatus() { return response_.first; }

  AuthenticatorImpl::GetAssertionCallback callback() {
    return std::move(callback_);
  }

 private:
  std::pair<AuthenticatorStatus, GetAssertionAuthenticatorResponsePtr>
      response_;
  base::OnceClosure closure_;
  AuthenticatorImpl::GetAssertionCallback callback_;
  base::RunLoop run_loop_;
};

}  // namespace

// Verify behavior for various combinations of origins and rp id's.
TEST_F(AuthenticatorImplTest, MakeCredentialOriginAndRpIds) {
  // These instances should return security errors (for circumstances
  // that would normally crash the renderer).
  for (const SecurityTestCases& test_case : kCrashingTestCases) {
    NavigateAndCommit(test_case.origin);
    AuthenticatorPtr authenticator = ConnectToAuthenticator();
    MakePublicKeyCredentialOptionsPtr options =
        GetTestMakePublicKeyCredentialOptions();
    DLOG(INFO) << "got options";
    options->relying_party->id = test_case.relying_party_id;
    DLOG(INFO) << options->relying_party->id;
    TestMakeCredentialCallback cb;
    DLOG(INFO) << "got callback";
    authenticator->MakeCredential(std::move(options), cb.callback());
    DLOG(INFO) << "called make cred";
    cb.WaitForCallback();
    DLOG(INFO) << "finished waiting";
    EXPECT_EQ(webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN,
              cb.GetResponseStatus());
  }

  // These instances pass the origin and relying party checks and return at
  // the algorithm check.
  for (const SecurityTestCases& test_case : kValidTestCases) {
    NavigateAndCommit(test_case.origin);
    AuthenticatorPtr authenticator = ConnectToAuthenticator();
    MakePublicKeyCredentialOptionsPtr options =
        GetTestMakePublicKeyCredentialOptions();
    options->relying_party->id = test_case.relying_party_id;
    options->public_key_parameters = GetTestPublicKeyCredentialParameters(123);

    TestMakeCredentialCallback cb;
    authenticator->MakeCredential(std::move(options), cb.callback());
    cb.WaitForCallback();
    EXPECT_EQ(webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR,
              cb.GetResponseStatus());
  }
}

// Test that service returns NOT_IMPLEMENTED_ERROR if no parameters contain
// a supported algorithm.
TEST_F(AuthenticatorImplTest, MakeCredentialNoSupportedAlgorithm) {
  SimulateNavigation(GURL(kTestOrigin1));
  AuthenticatorPtr authenticator = ConnectToAuthenticator();

  MakePublicKeyCredentialOptionsPtr options =
      GetTestMakePublicKeyCredentialOptions();
  options->public_key_parameters = GetTestPublicKeyCredentialParameters(123);

  TestMakeCredentialCallback cb;
  authenticator->MakeCredential(std::move(options), cb.callback());
  cb.WaitForCallback();
  EXPECT_EQ(webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR,
            cb.GetResponseStatus());
}

// Test that client data serializes to JSON properly.
TEST_F(AuthenticatorImplTest, TestSerializedRegisterClientData) {
  EXPECT_EQ(kTestRegisterClientDataJsonString,
            GetTestClientData(client_data::kCreateType).SerializeToJson());
}

TEST_F(AuthenticatorImplTest, TestSerializedSignClientData) {
  EXPECT_EQ(kTestSignClientDataJsonString,
            GetTestClientData(client_data::kGetType).SerializeToJson());
}

TEST_F(AuthenticatorImplTest, TestMakeCredentialTimeout) {
  SimulateNavigation(GURL(kTestOrigin1));
  MakePublicKeyCredentialOptionsPtr options =
      GetTestMakePublicKeyCredentialOptions();
  TestMakeCredentialCallback cb;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);
  service_manager::Connector::TestApi test_api(connector.get());
  test_api.OverrideBinderForTesting(
      device::mojom::kServiceName, device::mojom::HidManager::Name_,
      base::Bind(&device::FakeHidManager::AddBinding,
                 base::Unretained(fake_hid_manager.get())));

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto tick_clock = task_runner->GetMockTickClock();
  auto timer = std::make_unique<base::OneShotTimer>(tick_clock.get());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));

  authenticator->MakeCredential(std::move(options), cb.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  cb.WaitForCallback();
  EXPECT_EQ(webauth::mojom::AuthenticatorStatus::TIMED_OUT,
            cb.GetResponseStatus());
}

// Verify behavior for various combinations of origins and rp id's.
TEST_F(AuthenticatorImplTest, GetAssertionOriginAndRpIds) {
  // These instances should return security errors (for circumstances
  // that would normally crash the renderer).
  for (const SecurityTestCases& test_case : kCrashingTestCases) {
    NavigateAndCommit(test_case.origin);
    AuthenticatorPtr authenticator = ConnectToAuthenticator();
    PublicKeyCredentialRequestOptionsPtr options =
        GetTestPublicKeyCredentialRequestOptions();
    options->relying_party_id = test_case.relying_party_id;

    TestGetAssertionCallback cb;
    authenticator->GetAssertion(std::move(options), cb.callback());
    cb.WaitForCallback();
    EXPECT_EQ(webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN,
              cb.GetResponseStatus());
  }
}

TEST_F(AuthenticatorImplTest, TestGetAssertionTimeout) {
  SimulateNavigation(GURL(kTestOrigin1));
  PublicKeyCredentialRequestOptionsPtr options =
      GetTestPublicKeyCredentialRequestOptions();
  TestGetAssertionCallback cb;

  // Set up service_manager::Connector for tests.
  auto fake_hid_manager = std::make_unique<device::FakeHidManager>();
  service_manager::mojom::ConnectorRequest request;
  auto connector = service_manager::Connector::Create(&request);
  service_manager::Connector::TestApi test_api(connector.get());
  test_api.OverrideBinderForTesting(
      device::mojom::kServiceName, device::mojom::HidManager::Name_,
      base::Bind(&device::FakeHidManager::AddBinding,
                 base::Unretained(fake_hid_manager.get())));

  // Set up a timer for testing.
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  auto tick_clock = task_runner->GetMockTickClock();
  auto timer = std::make_unique<base::OneShotTimer>(tick_clock.get());
  timer->SetTaskRunner(task_runner);
  AuthenticatorPtr authenticator =
      ConnectToAuthenticator(connector.get(), std::move(timer));

  authenticator->GetAssertion(std::move(options), cb.callback());

  // Trigger timer.
  base::RunLoop().RunUntilIdle();
  task_runner->FastForwardBy(base::TimeDelta::FromMinutes(1));
  cb.WaitForCallback();
  EXPECT_EQ(webauth::mojom::AuthenticatorStatus::TIMED_OUT,
            cb.GetResponseStatus());
}
}  // namespace content
