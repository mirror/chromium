// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(OS_ANDROID)

#include "components/ntp_snippets/breaking_news/breaking_news_gcm_app_handler.h"

#include <memory>
#include <string>

#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/test/simple_test_clock.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/clock.h"
#include "base/time/tick_clock.h"
#include "build/build_config.h"
#include "components/gcm_driver/instance_id/instance_id.h"
#include "components/gcm_driver/instance_id/instance_id_driver.h"
#include "components/ntp_snippets/breaking_news/subscription_manager.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"

using base::TestMockTimeTaskRunner;
using base::TickClock;
using base::TimeTicks;
using gcm::InstanceIDHandler;
using instance_id::InstanceID;
using instance_id::InstanceIDDriver;
using testing::_;
using testing::AtMost;
using testing::NiceMock;
using testing::StrictMock;

namespace ntp_snippets {

namespace {

class MockSubscriptionManager : public SubscriptionManager {
 public:
  MockSubscriptionManager() = default;
  ~MockSubscriptionManager() override = default;

  MOCK_METHOD1(Subscribe, void(const std::string& token));
  MOCK_METHOD0(Unsubscribe, void());
  MOCK_METHOD0(IsSubscribed, bool());
  MOCK_METHOD1(Resubscribe, void(const std::string& new_token));
  MOCK_METHOD0(NeedsToResubscribe, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSubscriptionManager);
};

class MockInstanceIDHandler : public InstanceIDHandler {
 public:
  MockInstanceIDHandler() = default;
  ~MockInstanceIDHandler() override = default;

  MOCK_METHOD5(GetToken,
               void(const std::string& app_id,
                    const std::string& authorized_entity,
                    const std::string& scope,
                    const std::map<std::string, std::string>& options,
                    const GetTokenCallback& callback));
  MOCK_METHOD5(ValidateToken,
               void(const std::string& app_id,
                    const std::string& authorized_entity,
                    const std::string& scope,
                    const std::string& token,
                    const ValidateTokenCallback& callback));
  MOCK_METHOD4(DeleteToken,
               void(const std::string& app_id,
                    const std::string& authorized_entity,
                    const std::string& scope,
                    const DeleteTokenCallback& callback));
  MOCK_METHOD3(AddInstanceIDData,
               void(const std::string& app_id,
                    const std::string& instance_id,
                    const std::string& extra_data));
  MOCK_METHOD1(RemoveInstanceIDData, void(const std::string& app_id));
  MOCK_METHOD2(GetInstanceIDData,
               void(const std::string& app_id,
                    const GetInstanceIDDataCallback& callback));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockInstanceIDHandler);
};

// gcm::FakeGCMDriver just ignores all calls, but we prefer to fail when calling
// "non-implemented" methods.
class FakeCautiousGCMDriver : public gcm::GCMDriver {
 public:
  FakeCautiousGCMDriver(InstanceIDHandler* instance_id_handler)
      : GCMDriver(base::FilePath(), nullptr),
        instance_id_handler_(instance_id_handler) {}
  ~FakeCautiousGCMDriver() override = default;

  // GCMDriver implementation
  InstanceIDHandler* GetInstanceIDHandlerInternal() override {
    return instance_id_handler_;
  }

  // Not implemented methods.
  void ValidateRegistration(
      const std::string& app_id,
      const std::vector<std::string>& sender_ids,
      const std::string& registration_id,
      const ValidateRegistrationCallback& callback) override {
    // NOTREACHED could not be used, because it may be based on a DCHECK, which
    // when failing can leave a trybot in a bad state. ASSERT_FALSE could only
    // be used in functions returning void.
    EXPECT_TRUE(false) << "Not implemented";
  }
  void OnSignedIn() override { EXPECT_TRUE(false) << "Not implemented"; }
  void OnSignedOut() override { EXPECT_TRUE(false) << "Not implemented"; }
  void AddConnectionObserver(gcm::GCMConnectionObserver* observer) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void RemoveConnectionObserver(gcm::GCMConnectionObserver* observer) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void Enable() override { EXPECT_TRUE(false) << "Not implemented"; }
  void Disable() override { EXPECT_TRUE(false) << "Not implemented"; }
  gcm::GCMClient* GetGCMClientForTesting() const override {
    EXPECT_TRUE(false) << "Not implemented";
    return nullptr;
  }
  bool IsStarted() const override {
    EXPECT_TRUE(false) << "Not implemented";
    return false;
  }
  bool IsConnected() const override {
    EXPECT_TRUE(false) << "Not implemented";
    return false;
  }
  void GetGCMStatistics(const GetGCMStatisticsCallback& callback,
                        ClearActivityLogs clear_logs) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void SetGCMRecording(const GetGCMStatisticsCallback& callback,
                       bool recording) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void SetAccountTokens(const std::vector<gcm::GCMClient::AccountTokenInfo>&
                            account_tokens) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void UpdateAccountMapping(
      const gcm::AccountMapping& account_mapping) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void RemoveAccountMapping(const std::string& account_id) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  base::Time GetLastTokenFetchTime() override {
    EXPECT_TRUE(false) << "Not implemented";
    return base::Time();
  }
  void SetLastTokenFetchTime(const base::Time& time) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void WakeFromSuspendForHeartbeat(bool wake) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void AddHeartbeatInterval(const std::string& scope,
                            int interval_ms) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void RemoveHeartbeatInterval(const std::string& scope) override {
    EXPECT_TRUE(false) << "Not implemented";
  }

 protected:
  gcm::GCMClient::Result EnsureStarted(
      gcm::GCMClient::StartMode start_mode) override {
    EXPECT_TRUE(false) << "Not implemented";
    return gcm::GCMClient::Result::UNKNOWN_ERROR;
  }
  void RegisterImpl(const std::string& app_id,
                    const std::vector<std::string>& sender_ids) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void UnregisterImpl(const std::string& app_id) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void SendImpl(const std::string& app_id,
                const std::string& receiver_id,
                const gcm::OutgoingMessage& message) override {
    EXPECT_TRUE(false) << "Not implemented";
  }
  void RecordDecryptionFailure(const std::string& app_id,
                               gcm::GCMDecryptionResult result) override {
    EXPECT_TRUE(false) << "Not implemented";
  }

 private:
  InstanceIDHandler* instance_id_handler_;

  DISALLOW_COPY_AND_ASSIGN(FakeCautiousGCMDriver);
};

//                                                      day of week (0 = Sunday)
//                                                     /
const base::Time::Exploded kDummyNowTime = {2015, 11, 4, 25, 13, 46, 45};

ACTION_TEMPLATE(InvokeCallbackArgument,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_1_VALUE_PARAMS(p0)) {
  ::std::tr1::get<k>(args).Run(p0);
}

ACTION_TEMPLATE(InvokeCallbackArgument,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_2_VALUE_PARAMS(p0, p1)) {
  ::std::tr1::get<k>(args).Run(p0, p1);
}

base::Time GetDummyNow() {
  base::Time out_time;
  EXPECT_TRUE(base::Time::FromUTCExploded(kDummyNowTime, &out_time));
  return out_time;
}

base::TimeDelta GetTokenValidationPeriod() {
  return base::TimeDelta::FromHours(24);
}

}  // namespace

class BreakingNewsGCMAppHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    mock_instance_id_handler_ =
        base::MakeUnique<StrictMock<MockInstanceIDHandler>>();
    fake_gcm_driver_ = base::MakeUnique<FakeCautiousGCMDriver>(
        mock_instance_id_handler_.get());
    instance_id_driver_ =
        base::MakeUnique<InstanceIDDriver>(fake_gcm_driver_.get());
    // Our app handler obtains InstanceID through InstanceIDDriver. Then
    // InstanceID redirects all calls to InstanceIDHandler, which it obtains
    // from GCMDriver. As a result, in the setup above, all calls are handled by
    // mock_instance_id_handler_. InstanceIDDriver method for retrieving
    // InstanceID is not virtual, as a result we could not inject MockInstanceID
    // directly.
    BreakingNewsGCMAppHandler::RegisterProfilePrefs(
        utils_.pref_service()->registry());

    // This is called in InstanceID constructor.
    EXPECT_CALL(*mock_instance_id_handler_, GetInstanceIDData(_, _))
        .Times(AtMost(1))
        .WillOnce(InvokeCallbackArgument<1>("instance_id", /*extra_data=*/""));
  }

  std::unique_ptr<BreakingNewsGCMAppHandler> MakeHandler(
      std::unique_ptr<base::Clock> clock,
      scoped_refptr<base::SequencedTaskRunner> timer_task_runner,
      base::TickClock* timer_tick_clock) {
    auto wrapped_mock_subscription_manager =
        base::MakeUnique<NiceMock<MockSubscriptionManager>>();
    mock_subscription_manager_ = wrapped_mock_subscription_manager.get();

    return base::MakeUnique<BreakingNewsGCMAppHandler>(
        fake_gcm_driver_.get(), instance_id_driver_.get(), pref_service(),
        std::move(wrapped_mock_subscription_manager),
        BreakingNewsGCMAppHandler::ParseJSONCallback(), std::move(clock),
        timer_task_runner, timer_tick_clock);
  }

  PrefService* pref_service() { return utils_.pref_service(); }

  NiceMock<MockSubscriptionManager>* mock_subscription_manager() {
    return mock_subscription_manager_;
  }

  StrictMock<MockInstanceIDHandler>* mock_instance_id_handler() {
    return mock_instance_id_handler_.get();
  }

  base::MessageLoop* message_loop() { return &message_loop_; }

 private:
  base::MessageLoop message_loop_;
  test::RemoteSuggestionsTestUtils utils_;
  NiceMock<MockSubscriptionManager>* mock_subscription_manager_;
  std::unique_ptr<StrictMock<MockInstanceIDHandler>> mock_instance_id_handler_;
  std::unique_ptr<FakeCautiousGCMDriver> fake_gcm_driver_;
  std::unique_ptr<InstanceIDDriver> instance_id_driver_;
};

TEST_F(BreakingNewsGCMAppHandlerTest,
       ShouldValidateTokenImmediatelyIfValidationIsDue) {
  scoped_refptr<TestMockTimeTaskRunner> task_runner(
      new TestMockTimeTaskRunner(GetDummyNow(), TimeTicks::Now()));
  std::unique_ptr<TickClock> tick_clock(task_runner->GetMockTickClock());
  message_loop()->SetTaskRunner(task_runner);

  const base::Time last_validation = GetDummyNow() -
                                     GetTokenValidationPeriod() -
                                     base::TimeDelta::FromSeconds(1);
  pref_service()->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                           last_validation.ToInternalValue());
  // Let's omit receiving the token and put it there directly.
  pref_service()->SetString(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                            "token");

  auto handler =
      MakeHandler(task_runner->GetMockClock(), task_runner, tick_clock.get());

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, "token", _));
  handler->StartListening(
      base::Bind([](std::unique_ptr<base::Value> content) {}));
  task_runner->RunUntilIdle();
}

TEST_F(BreakingNewsGCMAppHandlerTest,
       ShouldScheduleTokenValidationIfNotYetDue) {
  scoped_refptr<TestMockTimeTaskRunner> task_runner(
      new TestMockTimeTaskRunner(GetDummyNow(), TimeTicks::Now()));
  std::unique_ptr<TickClock> tick_clock(task_runner->GetMockTickClock());
  message_loop()->SetTaskRunner(task_runner);

  const base::TimeDelta time_to_validation = base::TimeDelta::FromHours(1);
  const base::Time last_validation =
      GetDummyNow() - (GetTokenValidationPeriod() - time_to_validation);
  pref_service()->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                           last_validation.ToInternalValue());
  // Let's omit receiving the token and put it there directly.
  pref_service()->SetString(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                            "token");

  auto handler =
      MakeHandler(task_runner->GetMockClock(), task_runner, tick_clock.get());

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, _, _))
      .Times(0);
  handler->StartListening(
      base::Bind([](std::unique_ptr<base::Value> content) {}));
  task_runner->FastForwardBy(time_to_validation -
                             base::TimeDelta::FromSeconds(1));

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, "token", _));
  task_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
}

TEST_F(BreakingNewsGCMAppHandlerTest, ShouldNotValidateTokenBeforeListening) {
  scoped_refptr<TestMockTimeTaskRunner> task_runner(
      new TestMockTimeTaskRunner(GetDummyNow(), TimeTicks::Now()));
  std::unique_ptr<TickClock> tick_clock(task_runner->GetMockTickClock());
  message_loop()->SetTaskRunner(task_runner);

  const base::TimeDelta time_to_validation = base::TimeDelta::FromHours(1);
  const base::Time last_validation =
      GetDummyNow() - (GetTokenValidationPeriod() - time_to_validation);
  pref_service()->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                           last_validation.ToInternalValue());
  // Let's omit receiving the token and put it there directly.
  pref_service()->SetString(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                            "token");

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, _, _))
      .Times(0);
  auto handler =
      MakeHandler(task_runner->GetMockClock(), task_runner, tick_clock.get());
  task_runner->FastForwardBy(GetTokenValidationPeriod() * 10);
}

TEST_F(BreakingNewsGCMAppHandlerTest, ShouldNotValidateTokenAfterListening) {
  scoped_refptr<TestMockTimeTaskRunner> task_runner(
      new TestMockTimeTaskRunner(GetDummyNow(), TimeTicks::Now()));
  std::unique_ptr<TickClock> tick_clock(task_runner->GetMockTickClock());
  message_loop()->SetTaskRunner(task_runner);

  const base::TimeDelta time_to_validation = base::TimeDelta::FromHours(1);
  const base::Time last_validation =
      GetDummyNow() - (GetTokenValidationPeriod() - time_to_validation);
  pref_service()->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                           last_validation.ToInternalValue());
  // Let's omit receiving the token and put it there directly.
  pref_service()->SetString(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                            "token");

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, _, _))
      .Times(0);
  auto handler =
      MakeHandler(task_runner->GetMockClock(), task_runner, tick_clock.get());
  handler->StartListening(
      base::Bind([](std::unique_ptr<base::Value> content) {}));
  handler->StopListening();
  task_runner->FastForwardBy(GetTokenValidationPeriod() * 10);
}

TEST_F(BreakingNewsGCMAppHandlerTest,
       ShouldRescheduleTokenValidationWhenRetrievingToken) {
  scoped_refptr<TestMockTimeTaskRunner> task_runner(
      new TestMockTimeTaskRunner(GetDummyNow(), TimeTicks::Now()));
  std::unique_ptr<TickClock> tick_clock(task_runner->GetMockTickClock());
  message_loop()->SetTaskRunner(task_runner);

  const base::TimeDelta time_to_validation = base::TimeDelta::FromHours(1);
  const base::Time last_validation =
      GetDummyNow() - (GetTokenValidationPeriod() - time_to_validation);
  pref_service()->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                           last_validation.ToInternalValue());

  auto handler =
      MakeHandler(task_runner->GetMockClock(), task_runner, tick_clock.get());

  EXPECT_CALL(*mock_instance_id_handler(), GetToken(_, _, _, _, _))
      .WillOnce(InvokeCallbackArgument<4>(
          "token", /*result=*/gcm::GCMClient::Result::SUCCESS));
  handler->StartListening(
      base::Bind([](std::unique_ptr<base::Value> content) {}));

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, _, _))
      .Times(0);
  task_runner->FastForwardBy(GetTokenValidationPeriod() -
                             base::TimeDelta::FromSeconds(1));

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, "token", _));
  task_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
}

TEST_F(BreakingNewsGCMAppHandlerTest,
       ShouldScheduleNewTokenValidationAfterValidation) {
  scoped_refptr<TestMockTimeTaskRunner> task_runner(
      new TestMockTimeTaskRunner(GetDummyNow(), TimeTicks::Now()));
  std::unique_ptr<TickClock> tick_clock(task_runner->GetMockTickClock());
  message_loop()->SetTaskRunner(task_runner);

  const base::TimeDelta time_to_validation = base::TimeDelta::FromHours(1);
  const base::Time last_validation =
      GetDummyNow() - (GetTokenValidationPeriod() - time_to_validation);
  pref_service()->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                           last_validation.ToInternalValue());
  // Let's omit receiving the token and put it there directly.
  pref_service()->SetString(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                            "token");

  auto handler =
      MakeHandler(task_runner->GetMockClock(), task_runner, tick_clock.get());
  handler->StartListening(
      base::Bind([](std::unique_ptr<base::Value> content) {}));

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, "token", _))
      .WillOnce(InvokeCallbackArgument<4>(/*is_valid=*/true));
  task_runner->FastForwardBy(time_to_validation);

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, "token", _))
      .Times(0);
  task_runner->FastForwardBy(GetTokenValidationPeriod() -
                             base::TimeDelta::FromSeconds(1));

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, "token", _));
  task_runner->FastForwardBy(base::TimeDelta::FromSeconds(1));
}

TEST_F(BreakingNewsGCMAppHandlerTest,
       ShouldRetrieveNewTokenIfOldIsInvalidAfterValidation) {
  scoped_refptr<TestMockTimeTaskRunner> task_runner(
      new TestMockTimeTaskRunner(GetDummyNow(), TimeTicks::Now()));
  std::unique_ptr<TickClock> tick_clock(task_runner->GetMockTickClock());
  message_loop()->SetTaskRunner(task_runner);

  const base::Time last_validation =
      GetDummyNow() - 10 * GetTokenValidationPeriod();
  pref_service()->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                           last_validation.ToInternalValue());
  // Let's omit receiving the token and put it there directly.
  pref_service()->SetString(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                            "old_token");

  auto handler =
      MakeHandler(task_runner->GetMockClock(), task_runner, tick_clock.get());
  handler->StartListening(
      base::Bind([](std::unique_ptr<base::Value> content) {}));

  EXPECT_CALL(*mock_instance_id_handler(),
              ValidateToken(_, _, _, "old_token", _))
      .WillOnce(InvokeCallbackArgument<4>(/*is_valid=*/false));
  EXPECT_CALL(*mock_instance_id_handler(), GetToken(_, _, _, _, _))
      .WillOnce(InvokeCallbackArgument<4>(
          "new_token", /*result=*/gcm::GCMClient::Result::SUCCESS));
  EXPECT_CALL(*mock_subscription_manager(), Subscribe("new_token"));
  task_runner->RunUntilIdle();
}

TEST_F(BreakingNewsGCMAppHandlerTest,
       ShouldDoNothingIfOldTokenIsValidAfterValidation) {
  scoped_refptr<TestMockTimeTaskRunner> task_runner(
      new TestMockTimeTaskRunner(GetDummyNow(), TimeTicks::Now()));
  std::unique_ptr<TickClock> tick_clock(task_runner->GetMockTickClock());
  message_loop()->SetTaskRunner(task_runner);

  const base::Time last_validation =
      GetDummyNow() - 10 * GetTokenValidationPeriod();
  pref_service()->SetInt64(prefs::kBreakingNewsGCMLastTokenValidationTime,
                           last_validation.ToInternalValue());
  // Let's omit receiving the token and put it there directly.
  pref_service()->SetString(prefs::kBreakingNewsGCMSubscriptionTokenCache,
                            "token");

  auto handler =
      MakeHandler(task_runner->GetMockClock(), task_runner, tick_clock.get());
  handler->StartListening(
      base::Bind([](std::unique_ptr<base::Value> content) {}));

  EXPECT_CALL(*mock_instance_id_handler(), ValidateToken(_, _, _, "token", _))
      .WillOnce(InvokeCallbackArgument<4>(/*is_valid=*/true));
  EXPECT_CALL(*mock_instance_id_handler(), GetToken(_, _, _, _, _)).Times(0);
  EXPECT_CALL(*mock_subscription_manager(), Subscribe(_)).Times(0);
  task_runner->RunUntilIdle();
}

}  // namespace ntp_snippets

#endif  // defined(OS_ANDROID)
