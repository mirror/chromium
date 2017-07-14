// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/arc/arc_support_host.h"
#include "chrome/browser/chromeos/arc/auth/arc_active_directory_enrollment_token_fetcher.h"
#include "chrome/browser/chromeos/arc/extensions/fake_arc_support.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/policy/dm_token_storage.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/policy/cloud/test_request_interceptor.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_cryptohome_client.h"
#include "components/arc/arc_util.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "components/policy/core/common/policy_switches.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/network_delegate.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_job.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;

namespace arc {
namespace {

constexpr char kFakeDmToken[] = "fake-dm-token";
constexpr char kFakeEnrollmentToken[] = "fake-enrollment-token";
constexpr char kFakeUserId[] = "fake-user-id";
constexpr char kFakeAuthSessionId[] = "fake-auth-session-id";
constexpr char kFakeAdfsServerUrl[] = "http://example.com/adfs/ls/awesome.aspx";
constexpr char kNotYetFetched[] = "NOT-YET-FETCHED";
constexpr char kBadRequestHeader[] = "HTTP/1.1 400 Bad Request";

using Status = ArcActiveDirectoryEnrollmentTokenFetcher::Status;

std::string GetDmServerUrl() {
  policy ::BrowserPolicyConnectorChromeOS* const connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  return connector->device_management_service()->GetServerUrl();
}

Profile* GetProfile() {
  const user_manager::User* const primary_user =
      user_manager::UserManager::Get()->GetPrimaryUser();
  return chromeos::ProfileHelper::Get()->GetProfileByUser(primary_user);
}

// Encapsulates state while fetching enrollment tokens.
class FetchContext {
 public:
  explicit FetchContext(Status expected_status)
      : expected_status_(expected_status) {
    Reset();
  }

  // Resets state. The fetch status is reset to a state that is NOT expected.
  void Reset() {
    // Reset to a non-trivial, unexpected value.
    enrollment_token_ = kNotYetFetched;
    user_id_ = kNotYetFetched;

    // Reset to the value we do NOT expect.
    fetch_status_ =
        expected_status_ == Status::SUCCESS ? Status::FAILURE : Status::SUCCESS;
  }

  void Set(Status fetch_status,
           const std::string& enrollment_token,
           const std::string& user_id) {
    fetch_status_ = fetch_status;
    enrollment_token_ = enrollment_token;
    user_id_ = user_id;
  }

  // Verifies whether the current state matches the expected state, depending on
  // the value of |expected_status| passed into the constructor.
  void VerifyExpectations() {
    EXPECT_EQ(expected_status_, fetch_status_);
    if (expected_status_ == Status::SUCCESS) {
      EXPECT_EQ(kFakeEnrollmentToken, enrollment_token_);
      EXPECT_EQ(kFakeUserId, user_id_);
    } else {
      EXPECT_TRUE(enrollment_token_.empty());
      EXPECT_TRUE(user_id_.empty());
    }
  }

 private:
  Status expected_status_ = Status::SUCCESS;
  Status fetch_status_ = Status::SUCCESS;
  std::string enrollment_token_ = kNotYetFetched;
  std::string user_id_ = kNotYetFetched;
};

}  // namespace

// Checks whether |request| is a valid request to enroll a play user and returns
// the corresponding protobuf.
em::ActiveDirectoryEnrollPlayUserRequest CheckRequestAndGetEnrollRequest(
    net::URLRequest* request) {
  // Check the operation.
  std::string request_type;
  EXPECT_TRUE(
      net::GetValueForKeyInQuery(request->url(), "request", &request_type));
  EXPECT_EQ("active_directory_enroll_play_user", request_type);

  // Check content of request.
  const net::UploadDataStream* upload = request->get_upload();
  EXPECT_TRUE(upload);
  EXPECT_TRUE(upload->GetElementReaders());
  EXPECT_EQ(1u, upload->GetElementReaders()->size());
  EXPECT_TRUE((*upload->GetElementReaders())[0]->AsBytesReader());

  const net::UploadBytesElementReader* bytes_reader =
      (*upload->GetElementReaders())[0]->AsBytesReader();

  // Check the DMToken.
  net::HttpRequestHeaders request_headers = request->extra_request_headers();
  std::string value;
  EXPECT_TRUE(request_headers.GetHeader("Authorization", &value));
  EXPECT_EQ("GoogleDMToken token=" + std::string(kFakeDmToken), value);

  // Extract the actual request proto.
  em::DeviceManagementRequest parsed_request;
  EXPECT_TRUE(parsed_request.ParseFromArray(bytes_reader->bytes(),
                                            bytes_reader->length()));
  EXPECT_TRUE(parsed_request.has_active_directory_enroll_play_user_request());

  return parsed_request.active_directory_enroll_play_user_request();
}

net::URLRequestJob* SendResponse(net::URLRequest* request,
                                 net::NetworkDelegate* network_delegate,
                                 const em::DeviceManagementResponse& response) {
  std::string response_data;
  EXPECT_TRUE(response.SerializeToString(&response_data));
  return new net::URLRequestTestJob(request, network_delegate,
                                    net::URLRequestTestJob::test_headers(),
                                    response_data, true);
}

// If this gets called, the test will fail.
net::URLRequestJob* NotReachedResponseJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) {
  ADD_FAILURE() << "DMServer called when not expected";
  return nullptr;
}

// JobCallback for the interceptor to test non-SAML flow.
net::URLRequestJob* NonSamlResponseJob(net::URLRequest* request,
                                       net::NetworkDelegate* network_delegate) {
  CheckRequestAndGetEnrollRequest(request);

  // Response contains the enrollment token and user id.
  em::DeviceManagementResponse response;
  em::ActiveDirectoryEnrollPlayUserResponse* enroll_response =
      response.mutable_active_directory_enroll_play_user_response();
  enroll_response->set_enrollment_token(kFakeEnrollmentToken);
  enroll_response->set_user_id(kFakeUserId);

  return SendResponse(request, network_delegate, response);
}

// JobCallback for the interceptor to start the SAML flow.
net::URLRequestJob* InitiateSamlResponseJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) {
  em::ActiveDirectoryEnrollPlayUserRequest enroll_request =
      CheckRequestAndGetEnrollRequest(request);

  EXPECT_FALSE(enroll_request.has_auth_session_id());

  // Response contains only SAML parameters to initialize the SAML flow.
  em::DeviceManagementResponse response;
  em::ActiveDirectoryEnrollPlayUserResponse* enroll_response =
      response.mutable_active_directory_enroll_play_user_response();
  em::SamlParametersProto* saml_parameters =
      enroll_response->mutable_saml_parameters();
  saml_parameters->set_auth_session_id(kFakeAuthSessionId);
  saml_parameters->set_auth_redirect_url(kFakeAdfsServerUrl);

  return SendResponse(request, network_delegate, response);
}

// JobCallback returns a 400 Bad Request.
net::URLRequestJob* BadRequestResponseJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) {
  return new net::URLRequestTestJob(request, network_delegate,
                                    kBadRequestHeader, std::string(), true);
}

// JobCallback for the interceptor to end the SAML flow.
net::URLRequestJob* FinishSamlResponseJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) {
  DCHECK(request);
  em::ActiveDirectoryEnrollPlayUserRequest enroll_request =
      CheckRequestAndGetEnrollRequest(request);

  EXPECT_TRUE(enroll_request.has_auth_session_id());
  EXPECT_EQ(kFakeAuthSessionId, enroll_request.auth_session_id());

  // Response contains the enrollment token and user id.
  em::DeviceManagementResponse response;
  em::ActiveDirectoryEnrollPlayUserResponse* enroll_response =
      response.mutable_active_directory_enroll_play_user_response();
  enroll_response->set_enrollment_token(kFakeEnrollmentToken);
  enroll_response->set_user_id(kFakeUserId);

  return SendResponse(request, network_delegate, response);
}

class ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest
    : public InProcessBrowserTest,
      public ArcSupportHost::ErrorDelegate {
 public:
  bool CheckOnAdAuthPageAndVerify(ArcSupportHost::UIPage page) {
    if (page != ArcSupportHost::UIPage::AD_AUTH)
      return false;
    EXPECT_EQ(kFakeAdfsServerUrl,
              fake_arc_support_->get_ad_auth_federation_url());
    EXPECT_EQ(GetDmServerUrl(), fake_arc_support_->get_ad_auth_dm_url_prefix());
    return true;
  }

  // SAML flow callback that simulates SAML authentication success.
  void SimulateAdAuthSucceeds(ArcSupportHost::UIPage page) {
    if (CheckOnAdAuthPageAndVerify(page))
      fake_arc_support_->EmulateAuthSuccess("" /* auth_code unused */);
  }

  // SAML flow callback that simulates pressing the Cancel button or closing the
  // window.
  void SimulateAdAuthCancelled(base::RunLoop* run_loop,
                               ArcSupportHost::UIPage page) {
    if (!CheckOnAdAuthPageAndVerify(page))
      return;
    fake_arc_support_->Close();

    // Since ArcActiveDirectoryEnrollmentTokenFetcher won't call the
    // FetchCallback in case of an error, we break the runloop manually.
    run_loop->Quit();
  }

  // SAML flow callback that simulates SAML authentication failure.
  void SimulateAdAuthFails(base::RunLoop* run_loop,
                           ArcSupportHost::UIPage page) {
    if (!CheckOnAdAuthPageAndVerify(page))
      return;
    fake_arc_support_->EmulateAuthFailure("error");

    // Since ArcActiveDirectoryEnrollmentTokenFetcher won't call the
    // FetchCallback in case of an error, we break the runloop manually.
    run_loop->Quit();
  }

  // SAML flow callback that simulates SAML authentication retry.
  void SimulateAdAuthRetry(ArcSupportHost::UIPage page) {
    if (!CheckOnAdAuthPageAndVerify(page))
      return;
    saml_auth_count_++;

    if (saml_auth_count_ == 1) {
      // First saml auth attempt, trigger error and retry.
      fake_arc_support_->EmulateAuthFailure("error");
      EXPECT_EQ(ArcSupportHost::UIPage::ERROR, fake_arc_support_->ui_page());
      fake_arc_support_->ClickRetryButton();
    } else if (saml_auth_count_ == 2) {
      // Second saml auth attempt, trigger success.
      fake_arc_support_->EmulateAuthSuccess("" /* auth_code unused */);
    } else {
      NOTREACHED() << saml_auth_count_;
    }
  }

  // FetchCallback that receives fetch data and quits the loop.
  void OnEnrollmentTokenFetched(base::RunLoop* run_loop,
                                FetchContext* context,
                                Status fetch_status,
                                const std::string& enrollment_token,
                                const std::string& user_id) {
    context->Set(fetch_status, enrollment_token, user_id);
    run_loop->Quit();
  }

  // Callback to ArcActiveDirectoryEnrollmentTokenFetcher::Fetch that should
  // never be called.
  void OnEnrollmentTokenFetched_NotReached(Status fetch_status,
                                           const std::string& enrollment_token,
                                           const std::string& user_id) {
    NOTREACHED();
  }

 protected:
  ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest() = default;
  ~ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(policy::switches::kDeviceManagementUrl,
                                    "http://localhost");
    SetArcAvailableCommandLineForTesting(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    // Set fake cryptohome, because we want to fail DMToken retrieval
    auto cryptohome_client = base::MakeUnique<chromeos::FakeCryptohomeClient>();
    fake_cryptohome_client_ = cryptohome_client.get();
    chromeos::DBusThreadManager::GetSetterForTesting()->SetCryptohomeClient(
        std::move(cryptohome_client));
  }

  void SetUpOnMainThread() override {
    interceptor_ = base::MakeUnique<policy::TestRequestInterceptor>(
        "localhost", content::BrowserThread::GetTaskRunnerForThread(
                         content::BrowserThread::IO));

    support_host_ = base::MakeUnique<ArcSupportHost>(GetProfile());
    support_host_->SetErrorDelegate(this);
    fake_arc_support_ = base::MakeUnique<FakeArcSupport>(support_host_.get());
    token_fetcher_ = base::MakeUnique<ArcActiveDirectoryEnrollmentTokenFetcher>(
        support_host_.get());
  }

  void TearDownOnMainThread() override {
    token_fetcher_.reset();
    fake_arc_support_.reset();
    support_host_->SetErrorDelegate(nullptr);
    support_host_.reset();
    interceptor_.reset();
  }

  // Stores a correct (fake) DM token.
  // ArcActiveDirectoryEnrollmentTokenFetcher will succeed to fetch the DM
  // token.
  void StoreCorrectDmToken() {
    fake_cryptohome_client_->set_system_salt(
        chromeos::FakeCryptohomeClient::GetStubSystemSalt());
    fake_cryptohome_client_->SetServiceIsAvailable(true);
    // Store a fake DM token.
    base::RunLoop run_loop;
    auto dm_token_storage = base::MakeUnique<policy::DMTokenStorage>(
        g_browser_process->local_state());
    dm_token_storage->StoreDMToken(
        kFakeDmToken, base::BindOnce(
                          [](base::RunLoop* run_loop, bool success) {
                            EXPECT_TRUE(success);
                            run_loop->Quit();
                          },
                          &run_loop));
    // Because the StoreDMToken() operation interacts with the I/O thread,
    // RunUntilIdle() won't work here. Instead, use Run() and Quit()
    // explicitly in the callback.
    run_loop.Run();
  }

  // Does not store a correct DM token.
  // ArcActiveDirectoryEnrollmentTokenFetcher will fail to fetch the DM token.
  void FailDmToken() {
    fake_cryptohome_client_->set_system_salt(std::vector<uint8_t>());
    fake_cryptohome_client_->SetServiceIsAvailable(true);
  }

  void FetchEnrollmentTokenAndCheck(Status expected_status) {
    FetchContext context(expected_status);
    base::RunLoop run_loop;
    token_fetcher_->Fetch(
        base::Bind(&ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest::
                       OnEnrollmentTokenFetched,
                   base::Unretained(this), &run_loop, &context));
    // Because the Fetch() operation needs to interact with other threads,
    // RunUntilIdle() won't work here. Instead, use Run() and Quit()
    // explicitly in the callback.
    run_loop.Run();
    context.VerifyExpectations();
  }

  void FetchEnrollmentTokenAndExpectCallbackNotReached(
      base::RunLoop* run_loop) {
    FetchContext context(Status::FAILURE);  // Status is never actually used.
    token_fetcher_->Fetch(
        base::Bind(&ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest::
                       OnEnrollmentTokenFetched_NotReached,
                   base::Unretained(this)));
    run_loop->Run();
  }

  std::unique_ptr<policy::TestRequestInterceptor> interceptor_;
  std::unique_ptr<FakeArcSupport> fake_arc_support_;
  std::unique_ptr<ArcActiveDirectoryEnrollmentTokenFetcher> token_fetcher_;

 private:
  ArcSupportHost::AuthDelegate* GetAuthDelegate() {
    return static_cast<ArcSupportHost::AuthDelegate*>(token_fetcher_.get());
  }

 private:
  // ArcSupportHost:::ErrorDelegate:
  // Settings these prevents some DCHECK failures.
  void OnWindowClosed() override {}
  void OnRetryClicked() override {}
  void OnSendFeedbackClicked() override {}

  std::unique_ptr<ArcSupportHost> support_host_;
  // DBusThreadManager owns this.
  chromeos::FakeCryptohomeClient* fake_cryptohome_client_;
  int saml_auth_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest);
};  // namespace arc

// Non-SAML flow fetches valid enrollment token and user id.
IN_PROC_BROWSER_TEST_F(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest,
                       RequestAccountInfoSuccess) {
  StoreCorrectDmToken();
  interceptor_->PushJobCallback(base::Bind(&NonSamlResponseJob));
  FetchEnrollmentTokenAndCheck(Status::SUCCESS);
}

// Failure to fetch DM token leads to token fetch failure.
IN_PROC_BROWSER_TEST_F(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest,
                       DmTokenRetrievalFailed) {
  FailDmToken();
  interceptor_->PushJobCallback(base::Bind(NotReachedResponseJob));
  FetchEnrollmentTokenAndCheck(Status::FAILURE);
}

// Server responds with bad request and fails token fetch.
IN_PROC_BROWSER_TEST_F(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest,
                       RequestAccountInfoError) {
  StoreCorrectDmToken();
  interceptor_->PushJobCallback(
      policy::TestRequestInterceptor::BadRequestJob());
  FetchEnrollmentTokenAndCheck(Status::FAILURE);
}

// ARC disabled leads to failed token fetch.
IN_PROC_BROWSER_TEST_F(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest,
                       ArcDisabled) {
  StoreCorrectDmToken();
  interceptor_->PushJobCallback(
      policy::TestRequestInterceptor::HttpErrorJob("904 ARC Disabled"));
  FetchEnrollmentTokenAndCheck(Status::ARC_DISABLED);
}

// Successful enrollment token fetch including SAML authentication.
// SAML flow works as follows:
//   1) |token_fetcher_| sends a request to DM server to fetch the token.
//   2) DM server responds with an auth session id and a redirect URL to the
//      ADFS server. This is emulated in InitiateSamlResponseJob.
//   3) |token_fetcher_| sets the redirect URL in |fake_arc_support_| and
//   opens
//      the AD auth page, which triggers SimulateAdAuthSucceeds.
//   4) SimulateAdAuthSucceeds triggers the onAuthSucceeded event, which
//   causes
//      |arc_support_host_| to call into |token_fetcher_|'s OnAuthSucceeded.
//   5) |token_fetcher_| sends another request to DM server to fetch the
//   token,
//      this time with the auth session id from 2).
//   6) DM server responds with the token. This is emulated in
//      FinishSamlResponseJob.
IN_PROC_BROWSER_TEST_F(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest,
                       SamlFlowSuccess) {
  StoreCorrectDmToken();
  interceptor_->PushJobCallback(base::Bind(&InitiateSamlResponseJob));
  interceptor_->PushJobCallback(base::Bind(&FinishSamlResponseJob));
  fake_arc_support_->set_on_page_changed(
      base::Bind(&ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest::
                     SimulateAdAuthSucceeds,
                 base::Unretained(this)));
  FetchEnrollmentTokenAndCheck(Status::SUCCESS);
}

// SAML flow fails since the user closed the window or clicked the Cancel
// button.
IN_PROC_BROWSER_TEST_F(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest,
                       SamlFlowFailsUserCancelled) {
  StoreCorrectDmToken();
  interceptor_->PushJobCallback(base::Bind(&InitiateSamlResponseJob));
  base::RunLoop run_loop;
  fake_arc_support_->set_on_page_changed(
      base::Bind(&ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest::
                     SimulateAdAuthCancelled,
                 base::Unretained(this), &run_loop));

  // On user cancel, the FetchCallback should not be called (in fact, the
  // window closes silently).
  FetchEnrollmentTokenAndExpectCallbackNotReached(&run_loop);
}

// SAML flow fails because of an error.
IN_PROC_BROWSER_TEST_F(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest,
                       SamlFlowFailsError) {
  StoreCorrectDmToken();
  interceptor_->PushJobCallback(base::Bind(&InitiateSamlResponseJob));
  base::RunLoop run_loop;
  fake_arc_support_->set_on_page_changed(base::Bind(
      &ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest::SimulateAdAuthFails,
      base::Unretained(this), &run_loop));

  // Similar to user cancel, the callback to Fetch should never be called.
  // Instead, we should end up on the error page.
  FetchEnrollmentTokenAndExpectCallbackNotReached(&run_loop);
  EXPECT_EQ(ArcSupportHost::UIPage::ERROR, fake_arc_support_->ui_page());
}

// SAML flow fails first during initial DM server request, but the retry
// works.
IN_PROC_BROWSER_TEST_F(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest,
                       SamlFlowSucceedsWithDmRetry) {
  StoreCorrectDmToken();
  interceptor_->PushJobCallback(base::Bind(&BadRequestResponseJob));
  FetchEnrollmentTokenAndCheck(Status::FAILURE);

  interceptor_->PushJobCallback(base::Bind(&InitiateSamlResponseJob));
  interceptor_->PushJobCallback(base::Bind(&FinishSamlResponseJob));
  fake_arc_support_->set_on_page_changed(
      base::Bind(&ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest::
                     SimulateAdAuthSucceeds,
                 base::Unretained(this)));
  FetchEnrollmentTokenAndCheck(Status::SUCCESS);
}

// SAML flow fails first during SAML auth, but the retry works.
IN_PROC_BROWSER_TEST_F(ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest,
                       SamlFlowSucceedsWithAuthRetry) {
  StoreCorrectDmToken();
  interceptor_->PushJobCallback(base::Bind(&InitiateSamlResponseJob));
  interceptor_->PushJobCallback(base::Bind(&InitiateSamlResponseJob));
  interceptor_->PushJobCallback(base::Bind(&FinishSamlResponseJob));
  fake_arc_support_->set_on_page_changed(base::Bind(
      &ArcActiveDirectoryEnrollmentTokenFetcherBrowserTest::SimulateAdAuthRetry,
      base::Unretained(this)));
  FetchEnrollmentTokenAndCheck(Status::SUCCESS);
  EXPECT_EQ(0u, interceptor_->GetPendingSize());
}

}  // namespace arc
