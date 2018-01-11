// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

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
  const base::Optional<std::string> relying_party_id;
  const webauth::mojom::AuthenticatorStatus status;
} SecurityTestCases;

constexpr char kTestOrigin1[] = "https://a.google.com";

// Test data. CBOR test data can be built using the given diagnostic strings
// and the utility at "http://cbor.me/".
constexpr int32_t kCoseEs256 = -7;

constexpr char kTestRelyingPartyId[] = "google.com";

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
    {GURL(kTestOrigin1), base::nullopt,
     webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR}, /* no rp id */
    {GURL(kTestOrigin1), std::string(kTestRelyingPartyId),
     webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR},
    {GURL("https://foo.bar.google.com"), std::string("bar.google.com"),
     webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR}, /* subdomain */
};

const SecurityTestCases kCrashingTestCases[] = {
    {GURL(kTestOrigin1), std::string("localhost"),
     webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN},
    {GURL("about:blank"), base::nullopt,
     webauth::mojom::AuthenticatorStatus::INSECURE_ORIGIN},
    {GURL("about:blank"), std::string(kTestRelyingPartyId),
     webauth::mojom::AuthenticatorStatus::INSECURE_ORIGIN},
    {GURL("about:blank"), std::string("localhost"),
     webauth::mojom::AuthenticatorStatus::INSECURE_ORIGIN},
    {GURL("data:foobar"), base::nullopt,
     webauth::mojom::AuthenticatorStatus::INSECURE_ORIGIN},
    {GURL("data:foobar"), std::string(kTestRelyingPartyId),
     webauth::mojom::AuthenticatorStatus::INSECURE_ORIGIN},
    {GURL("data:foobar"), std::string("localhost"),
     webauth::mojom::AuthenticatorStatus::INSECURE_ORIGIN},
    {GURL("data:text/html,opaque"), base::nullopt,
     webauth::mojom::AuthenticatorStatus::INSECURE_ORIGIN},
    {GURL("data:text/html,opaque"), std::string(kTestRelyingPartyId),
     webauth::mojom::AuthenticatorStatus::INSECURE_ORIGIN},
    {GURL("data:text/html,opaque"), std::string("localhost"),
     webauth::mojom::AuthenticatorStatus::INSECURE_ORIGIN},
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
  options->relying_party_id = std::string("localhost");
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
  std::pair<AuthenticatorStatus, MakeCredentialAuthenticatorResponsePtr>&
  WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return response_;
  }

  AuthenticatorImpl::MakeCredentialCallback callback() {
    return std::move(callback_);
  }

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
  std::pair<AuthenticatorStatus, GetAssertionAuthenticatorResponsePtr>&
  WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return response_;
  }

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
    options->relying_party->id = test_case.relying_party_id;

    TestMakeCredentialCallback cb;
    authenticator->MakeCredential(std::move(options), cb.callback());
    std::pair<webauth::mojom::AuthenticatorStatus,
              webauth::mojom::MakeCredentialAuthenticatorResponsePtr>&
        response = cb.WaitForCallback();
    EXPECT_EQ(test_case.status, response.first);
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
    std::pair<webauth::mojom::AuthenticatorStatus,
              webauth::mojom::MakeCredentialAuthenticatorResponsePtr>&
        response = cb.WaitForCallback();
    EXPECT_EQ(test_case.status, response.first);
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
  std::pair<webauth::mojom::AuthenticatorStatus,
            webauth::mojom::MakeCredentialAuthenticatorResponsePtr>& response =
      cb.WaitForCallback();
  EXPECT_EQ(webauth::mojom::AuthenticatorStatus::NOT_SUPPORTED_ERROR,
            response.first);
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
  std::pair<webauth::mojom::AuthenticatorStatus,
            webauth::mojom::MakeCredentialAuthenticatorResponsePtr>& response =
      cb.WaitForCallback();
  EXPECT_EQ(webauth::mojom::AuthenticatorStatus::TIMEOUT, response.first);
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
    std::pair<webauth::mojom::AuthenticatorStatus,
              webauth::mojom::GetAssertionAuthenticatorResponsePtr>& response =
        cb.WaitForCallback();
    EXPECT_EQ(test_case.status, response.first);
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
    std::pair<webauth::mojom::AuthenticatorStatus,
              webauth::mojom::MakeCredentialAuthenticatorResponsePtr>&
        response = cb.WaitForCallback();
    EXPECT_EQ(test_case.status, response.first);
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
  std::pair<webauth::mojom::AuthenticatorStatus,
            webauth::mojom::GetAssertionAuthenticatorResponsePtr>& response =
      cb.WaitForCallback();
  EXPECT_EQ(webauth::mojom::AuthenticatorStatus::TIMEOUT, response.first);
}
}  // namespace content
