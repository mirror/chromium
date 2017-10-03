// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include <unordered_map>

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/browser/ui/webui/interventions_internals/interventions_internals.mojom.h"
#include "components/previews/core/previews_features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// The key for the mapping for the enabled/disabled status map.
constexpr char kAMPRedirectionPreviews[] = "ampPreviews";
constexpr char kClientLoFiPreviews[] = "clientLoFiPreviews";
constexpr char kOfflinePreviews[] = "offlinePreviews";

// Description for statuses.
constexpr char kAmpRedirectionDescription[] = "AMP Previews";
constexpr char kClientLoFiDescription[] = "Client LoFi Previews";
constexpr char kOfflineDesciption[] = "Offline Previews";

std::unordered_map<std::string, mojom::PreviewsStatusPtr> passedInParams;
std::vector<mojom::MessageLogPtr> passedInLogs;

void MockGetPreviewsEnabledCallback(
    std::unordered_map<std::string, mojom::PreviewsStatusPtr> params) {
  passedInParams = std::move(params);
}

void MockGetMessageLogsCallback(std::vector<mojom::MessageLogPtr> params) {
  passedInLogs = std::move(params);
}

class TestPreviewsLogger : public previews::PreviewsLogger {
 public:
  TestPreviewsLogger() {}

  std::vector<previews::PreviewsLogger::MessageLog> log_messages()
      const override {
    return logs_;
  }

  void SetLoggerMessageLogs(
      std::vector<previews::PreviewsLogger::MessageLog> logs) {
    logs_ = std::move(logs);
  }

 private:
  std::vector<previews::PreviewsLogger::MessageLog> logs_;
};

class InterventionsInternalsPageHandlerTest : public testing::Test {
 public:
  InterventionsInternalsPageHandlerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {}

  ~InterventionsInternalsPageHandlerTest() override {}

  void SetUp() override {
    scoped_task_environment_.RunUntilIdle();

    request = mojo::MakeRequest(&pageHandlerPtr);
    logger_ = new TestPreviewsLogger();
    pageHandler.reset(
        new InterventionsInternalsPageHandler(std::move(request), logger_));

    scoped_feature_list_ = base::MakeUnique<base::test::ScopedFeatureList>();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  TestPreviewsLogger* logger_;
  mojom::InterventionsInternalsPageHandlerPtr pageHandlerPtr;
  mojom::InterventionsInternalsPageHandlerRequest request;
  std::unique_ptr<InterventionsInternalsPageHandler> pageHandler;
  std::unique_ptr<base::test::ScopedFeatureList> scoped_feature_list_;
};

}  // namespace

TEST_F(InterventionsInternalsPageHandlerTest, CorrectSizeOfPassingParameters) {
  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));

  constexpr size_t expected = 3;
  EXPECT_EQ(expected, passedInParams.size());
}

TEST_F(InterventionsInternalsPageHandlerTest, AMPRedirectionDisabled) {
  // Init with kAMPRedirection disabled.
  scoped_feature_list_->InitWithFeatures({},
                                         {previews::features::kAMPRedirection});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto amp_redirection = passedInParams.find(kAMPRedirectionPreviews);
  ASSERT_NE(passedInParams.end(), amp_redirection);
  EXPECT_EQ(kAmpRedirectionDescription, amp_redirection->second->description);
  EXPECT_FALSE(amp_redirection->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, AMPRedirectionEnabled) {
  // Init with kAMPRedirection enabled.
  scoped_feature_list_->InitWithFeatures({previews::features::kAMPRedirection},
                                         {});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto amp_redirection = passedInParams.find(kAMPRedirectionPreviews);
  ASSERT_NE(passedInParams.end(), amp_redirection);
  EXPECT_EQ(kAmpRedirectionDescription, amp_redirection->second->description);
  EXPECT_TRUE(amp_redirection->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, ClientLoFiDisabled) {
  // Init with kClientLoFi disabled.
  scoped_feature_list_->InitWithFeatures({}, {previews::features::kClientLoFi});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto client_lofi = passedInParams.find(kClientLoFiPreviews);
  ASSERT_NE(passedInParams.end(), client_lofi);
  EXPECT_EQ(kClientLoFiDescription, client_lofi->second->description);
  EXPECT_FALSE(client_lofi->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, ClientLoFiEnabled) {
  // Init with kClientLoFi enabled.
  scoped_feature_list_->InitWithFeatures({previews::features::kClientLoFi}, {});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto client_lofi = passedInParams.find(kClientLoFiPreviews);
  ASSERT_NE(passedInParams.end(), client_lofi);
  EXPECT_EQ(kClientLoFiDescription, client_lofi->second->description);
  EXPECT_TRUE(client_lofi->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, OfflinePreviewsDisabled) {
  // Init with kOfflinePreviews disabled.
  scoped_feature_list_->InitWithFeatures(
      {}, {previews::features::kOfflinePreviews});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto offline_previews = passedInParams.find(kOfflinePreviews);
  ASSERT_NE(passedInParams.end(), offline_previews);
  EXPECT_EQ(kOfflineDesciption, offline_previews->second->description);
  EXPECT_FALSE(offline_previews->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, OfflinePreviewsEnabled) {
  // Init with kOfflinePreviews enabled.
  scoped_feature_list_->InitWithFeatures({previews::features::kOfflinePreviews},
                                         {});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto offline_previews = passedInParams.find(kOfflinePreviews);
  ASSERT_NE(passedInParams.end(), offline_previews);
  EXPECT_TRUE(offline_previews->second);
  EXPECT_EQ(kOfflineDesciption, offline_previews->second->description);
  EXPECT_TRUE(offline_previews->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, GetMessageLogsPassCorrectParams) {
  previews::PreviewsLogger::MessageLog messages[] = {
      previews::PreviewsLogger::MessageLog("Event_a", "Description a",
                                           GURL("http://www.url_a.com/url_a"),
                                           base::Time::Now()),
      previews::PreviewsLogger::MessageLog("Event_b", "Description b",
                                           GURL("http://www.url_b.com/url_b"),
                                           base::Time::Now()),
      previews::PreviewsLogger::MessageLog("Event_c", "Description c",
                                           GURL("http://www.url_c.com/url_c"),
                                           base::Time::Now()),
  };

  std::vector<previews::PreviewsLogger::MessageLog> logs;
  for (auto message : messages) {
    logs.push_back(message);
  }
  logger_->SetLoggerMessageLogs(logs);

  pageHandler->GetMessageLogs(base::Bind(&MockGetMessageLogsCallback));
  const size_t expected_size = 3;
  EXPECT_EQ(expected_size, passedInLogs.size());

  for (size_t i = 0; i < passedInLogs.size(); i++) {
    EXPECT_EQ(messages[i].event_type, passedInLogs[i]->type);
    EXPECT_EQ(messages[i].event_description, passedInLogs[i]->description);
    EXPECT_EQ(messages[i].url.spec(), passedInLogs[i]->url);
  }
}
