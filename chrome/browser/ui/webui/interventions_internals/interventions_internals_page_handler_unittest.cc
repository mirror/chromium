// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/interventions_internals/interventions_internals_page_handler.h"

#include <unordered_map>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/browser/ui/webui/interventions_internals/interventions_internals.mojom.h"
#include "components/previews/core/previews_features.h"
#include "mojo/public/cpp/bindings/binding.h"
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

// Get the string representation of the time in the "d/M/y - h:m::s" format.
std::string ConvertTimeToString(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::StringPrintf("%d/%d/%d - %d:%d:%d", exploded.month,
                            exploded.day_of_month, exploded.year, exploded.hour,
                            exploded.minute, exploded.second);
}

// The map that would be passed to the callback in GetPreviewsEnabledCallback.
std::unordered_map<std::string, mojom::PreviewsStatusPtr> passedInMap;

void MockGetPreviewsEnabledCallback(
    std::unordered_map<std::string, mojom::PreviewsStatusPtr> params) {
  passedInMap = std::move(params);
}

// Mock class that would be pass into the InterventionsInternalsPageHandler by
// calling its SetClientPage method. Used to test that the PageHandler
// actually invokes the page's LogNewMessage method with the correct message.
class TestInterventionsInternalsPage
    : public mojom::InterventionsInternalsPage {
 public:
  explicit TestInterventionsInternalsPage(
      mojom::InterventionsInternalsPageRequest request)
      : binding_(this, std::move(request)) {}

  ~TestInterventionsInternalsPage() override {}

  // mojom::InterventionsInternalsPage:
  void LogNewMessage(mojom::MessageLogPtr message) override {
    message_ = std::move(message);
  }

  // Expose passed in message in LogNewMessage for testing.
  mojom::MessageLogPtr message() { return std::move(message_); }

 private:
  mojo::Binding<mojom::InterventionsInternalsPage> binding_;

  // The MessageLogPtr passed in LogNewMessage method.
  mojom::MessageLogPtr message_;
};

class InterventionsInternalsPageHandlerTest : public testing::Test {
 public:
  InterventionsInternalsPageHandlerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {}

  ~InterventionsInternalsPageHandlerTest() override {}

  void SetUp() override {
    scoped_task_environment_.RunUntilIdle();

    mojom::InterventionsInternalsPageHandlerPtr pageHandlerPtr;
    handlerRequest = mojo::MakeRequest(&pageHandlerPtr);
    pageHandler = base::MakeUnique<InterventionsInternalsPageHandler>(
        std::move(handlerRequest));

    mojom::InterventionsInternalsPagePtr pagePtr;
    pageRequest = mojo::MakeRequest(&pagePtr);
    page = base::MakeUnique<TestInterventionsInternalsPage>(
        std::move(pageRequest));

    pageHandler->SetClientPage(std::move(pagePtr));

    scoped_feature_list_ = base::MakeUnique<base::test::ScopedFeatureList>();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // InterventionsInternalPageHandler's variables.
  mojom::InterventionsInternalsPageHandlerRequest handlerRequest;
  std::unique_ptr<InterventionsInternalsPageHandler> pageHandler;

  // InterventionsInternalPage's variables.
  mojom::InterventionsInternalsPageRequest pageRequest;
  std::unique_ptr<TestInterventionsInternalsPage> page;

  std::unique_ptr<base::test::ScopedFeatureList> scoped_feature_list_;
};

TEST_F(InterventionsInternalsPageHandlerTest, CorrectSizeOfPassingParameters) {
  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));

  constexpr size_t expected = 3;
  EXPECT_EQ(expected, passedInMap.size());
}

TEST_F(InterventionsInternalsPageHandlerTest, AMPRedirectionDisabled) {
  // Init with kAMPRedirection disabled.
  scoped_feature_list_->InitWithFeatures({},
                                         {previews::features::kAMPRedirection});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto amp_redirection = passedInMap.find(kAMPRedirectionPreviews);
  ASSERT_NE(passedInMap.end(), amp_redirection);
  EXPECT_EQ(kAmpRedirectionDescription, amp_redirection->second->description);
  EXPECT_FALSE(amp_redirection->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, AMPRedirectionEnabled) {
  // Init with kAMPRedirection enabled.
  scoped_feature_list_->InitWithFeatures({previews::features::kAMPRedirection},
                                         {});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto amp_redirection = passedInMap.find(kAMPRedirectionPreviews);
  ASSERT_NE(passedInMap.end(), amp_redirection);
  EXPECT_EQ(kAmpRedirectionDescription, amp_redirection->second->description);
  EXPECT_TRUE(amp_redirection->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, ClientLoFiDisabled) {
  // Init with kClientLoFi disabled.
  scoped_feature_list_->InitWithFeatures({}, {previews::features::kClientLoFi});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto client_lofi = passedInMap.find(kClientLoFiPreviews);
  ASSERT_NE(passedInMap.end(), client_lofi);
  EXPECT_EQ(kClientLoFiDescription, client_lofi->second->description);
  EXPECT_FALSE(client_lofi->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, ClientLoFiEnabled) {
  // Init with kClientLoFi enabled.
  scoped_feature_list_->InitWithFeatures({previews::features::kClientLoFi}, {});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto client_lofi = passedInMap.find(kClientLoFiPreviews);
  ASSERT_NE(passedInMap.end(), client_lofi);
  EXPECT_EQ(kClientLoFiDescription, client_lofi->second->description);
  EXPECT_TRUE(client_lofi->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, OfflinePreviewsDisabled) {
  // Init with kOfflinePreviews disabled.
  scoped_feature_list_->InitWithFeatures(
      {}, {previews::features::kOfflinePreviews});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto offline_previews = passedInMap.find(kOfflinePreviews);
  ASSERT_NE(passedInMap.end(), offline_previews);
  EXPECT_EQ(kOfflineDesciption, offline_previews->second->description);
  EXPECT_FALSE(offline_previews->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, OfflinePreviewsEnabled) {
  // Init with kOfflinePreviews enabled.
  scoped_feature_list_->InitWithFeatures({previews::features::kOfflinePreviews},
                                         {});

  pageHandler->GetPreviewsEnabled(
      base::BindOnce(&MockGetPreviewsEnabledCallback));
  auto offline_previews = passedInMap.find(kOfflinePreviews);
  ASSERT_NE(passedInMap.end(), offline_previews);
  EXPECT_TRUE(offline_previews->second);
  EXPECT_EQ(kOfflineDesciption, offline_previews->second->description);
  EXPECT_TRUE(offline_previews->second->enabled);
}

TEST_F(InterventionsInternalsPageHandlerTest, OnNewMessageLogAddedPostToPage) {
  const previews::PreviewsLogger::MessageLog expected_messages[] = {
      previews::PreviewsLogger::MessageLog("Event_a", "Some description a",
                                           GURL("http://www.url_a.com/url_a"),
                                           base::Time::Now()),
      previews::PreviewsLogger::MessageLog("Event_b", "Some description b",
                                           GURL("http://www.url_b.com/url_b"),
                                           base::Time::Now()),
      previews::PreviewsLogger::MessageLog("Event_c", "Some description c",
                                           GURL("http://www.url_c.com/url_c"),
                                           base::Time::Now()),
  };

  for (auto message : expected_messages) {
    pageHandler->OnNewMessageLogAdded(message);
    base::RunLoop().RunUntilIdle();

    mojom::MessageLogPtr actual = page->message();
    EXPECT_EQ(message.event_type, actual->type);
    EXPECT_EQ(message.event_description, actual->description);
    EXPECT_EQ(message.url, actual->url);

    std::string expected_time = ConvertTimeToString(message.time);
    EXPECT_EQ(expected_time, actual->time);
  }
}

}  // namespace
