// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/browser_feature_extractor.h"

#include <map>
#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/time.h"
#include "chrome/common/safe_browsing/csd.pb.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/history/history_backend.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/client_side_detection_service.h"
#include "chrome/test/testing_profile.h"
#include "content/browser/browser_thread.h"
#include "content/browser/renderer_host/test_render_view_host.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/test_tab_contents.h"
#include "content/common/page_transition_types.h"
#include "googleurl/src/gurl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Return;
using ::testing::StrictMock;

namespace safe_browsing {
namespace {
class MockClientSideDetectionService : public ClientSideDetectionService {
 public:
  MockClientSideDetectionService() : ClientSideDetectionService(NULL) {}
  virtual ~MockClientSideDetectionService() {};

  MOCK_CONST_METHOD1(IsBadIpAddress, bool(const std::string&));
};
}  // namespace

class BrowserFeatureExtractorTest : public RenderViewHostTestHarness {
 protected:
  BrowserFeatureExtractorTest()
      : ui_thread_(BrowserThread::UI, &message_loop_) {
  }

  virtual void SetUp() {
    RenderViewHostTestHarness::SetUp();
    profile_->CreateHistoryService(true /* delete_file */, false /* no_db */);
    service_.reset(new StrictMock<MockClientSideDetectionService>());
    extractor_.reset(new BrowserFeatureExtractor(contents(), service_.get()));
    num_pending_ = 0;
    browse_info_.reset(new BrowseInfo);
  }

  virtual void TearDown() {
    extractor_.reset();
    profile_->DestroyHistoryService();
    RenderViewHostTestHarness::TearDown();
    ASSERT_EQ(0, num_pending_);
  }

  HistoryService* history_service() {
    return profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  }

  bool ExtractFeatures(ClientPhishingRequest* request) {
    StartExtractFeatures(request);
    MessageLoop::current()->Run();
    EXPECT_EQ(1U, success_.count(request));
    return success_.count(request) ? success_[request] : false;
  }

  void StartExtractFeatures(ClientPhishingRequest* request) {
    success_.erase(request);
    ++num_pending_;
    extractor_->ExtractFeatures(
        browse_info_.get(),
        request,
        NewCallback(this,
                    &BrowserFeatureExtractorTest::ExtractFeaturesDone));
  }

  void GetFeatureMap(const ClientPhishingRequest& request,
                     std::map<std::string, double>* features) {
    for (int i = 0; i < request.non_model_feature_map_size(); ++i) {
      const ClientPhishingRequest::Feature& feature =
          request.non_model_feature_map(i);
      EXPECT_EQ(0U, features->count(feature.name()));
      (*features)[feature.name()] = feature.value();
    }
  }

  BrowserThread ui_thread_;
  int num_pending_;
  scoped_ptr<BrowserFeatureExtractor> extractor_;
  std::map<ClientPhishingRequest*, bool> success_;
  scoped_ptr<BrowseInfo> browse_info_;
  scoped_ptr<MockClientSideDetectionService> service_;


 private:
  void ExtractFeaturesDone(bool success, ClientPhishingRequest* request) {
    ASSERT_EQ(0U, success_.count(request));
    success_[request] = success;
    if (--num_pending_ == 0) {
      MessageLoop::current()->Quit();
    }
  }
};

TEST_F(BrowserFeatureExtractorTest, UrlNotInHistory) {
  ClientPhishingRequest request;
  request.set_url("http://www.google.com/");
  request.set_client_score(0.5);
  EXPECT_FALSE(ExtractFeatures(&request));
}

TEST_F(BrowserFeatureExtractorTest, RequestNotInitialized) {
  ClientPhishingRequest request;
  request.set_url("http://www.google.com/");
  // Request is missing the score value.
  EXPECT_FALSE(ExtractFeatures(&request));
}

TEST_F(BrowserFeatureExtractorTest, UrlInHistory) {
  history_service()->AddPage(GURL("http://www.foo.com/bar.html"),
                             history::SOURCE_BROWSED);
  history_service()->AddPage(GURL("https://www.foo.com/gaa.html"),
                             history::SOURCE_BROWSED);  // same host HTTPS.
  history_service()->AddPage(GURL("http://www.foo.com/gaa.html"),
                             history::SOURCE_BROWSED);  // same host HTTP.
  history_service()->AddPage(GURL("http://bar.foo.com/gaa.html"),
                             history::SOURCE_BROWSED);  // different host.
  history_service()->AddPage(GURL("http://www.foo.com/bar.html?a=b"),
                             base::Time::Now() - base::TimeDelta::FromHours(23),
                             NULL, 0, GURL(), PageTransition::LINK,
                             history::RedirectList(), history::SOURCE_BROWSED,
                             false);
  history_service()->AddPage(GURL("http://www.foo.com/bar.html"),
                             base::Time::Now() - base::TimeDelta::FromHours(25),
                             NULL, 0, GURL(), PageTransition::TYPED,
                             history::RedirectList(), history::SOURCE_BROWSED,
                             false);
  history_service()->AddPage(GURL("https://www.foo.com/goo.html"),
                             base::Time::Now() - base::TimeDelta::FromDays(5),
                             NULL, 0, GURL(), PageTransition::TYPED,
                             history::RedirectList(), history::SOURCE_BROWSED,
                             false);

  ClientPhishingRequest request;
  request.set_url("http://www.foo.com/bar.html");
  request.set_client_score(0.5);
  EXPECT_TRUE(ExtractFeatures(&request));
  std::map<std::string, double> features;
  GetFeatureMap(request, &features);

  EXPECT_EQ(10U, features.size());
  EXPECT_DOUBLE_EQ(2.0, features[features::kUrlHistoryVisitCount]);
  EXPECT_DOUBLE_EQ(1.0,
                   features[features::kUrlHistoryVisitCountMoreThan24hAgo]);
  EXPECT_DOUBLE_EQ(1.0, features[features::kUrlHistoryTypedCount]);
  EXPECT_DOUBLE_EQ(1.0, features[features::kUrlHistoryLinkCount]);
  EXPECT_DOUBLE_EQ(4.0, features[features::kHttpHostVisitCount]);
  EXPECT_DOUBLE_EQ(2.0, features[features::kHttpsHostVisitCount]);
  EXPECT_DOUBLE_EQ(1.0, features[features::kFirstHttpHostVisitMoreThan24hAgo]);
  EXPECT_DOUBLE_EQ(1.0, features[features::kFirstHttpsHostVisitMoreThan24hAgo]);

  request.Clear();
  request.set_url("http://bar.foo.com/gaa.html");
  request.set_client_score(0.5);
  EXPECT_TRUE(ExtractFeatures(&request));
  features.clear();
  GetFeatureMap(request, &features);
  EXPECT_EQ(9U, features.size());
  EXPECT_DOUBLE_EQ(1.0, features[features::kUrlHistoryVisitCount]);
  EXPECT_DOUBLE_EQ(0.0,
                   features[features::kUrlHistoryVisitCountMoreThan24hAgo]);
  EXPECT_DOUBLE_EQ(0.0, features[features::kUrlHistoryTypedCount]);
  EXPECT_DOUBLE_EQ(1.0, features[features::kUrlHistoryLinkCount]);
  EXPECT_DOUBLE_EQ(1.0, features[features::kHttpHostVisitCount]);
  EXPECT_DOUBLE_EQ(0.0, features[features::kHttpsHostVisitCount]);
  EXPECT_DOUBLE_EQ(0.0, features[features::kFirstHttpHostVisitMoreThan24hAgo]);
  EXPECT_FALSE(features.count(features::kFirstHttpsHostVisitMoreThan24hAgo));
}

TEST_F(BrowserFeatureExtractorTest, MultipleRequestsAtOnce) {
  history_service()->AddPage(GURL("http://www.foo.com/bar.html"),
                             history::SOURCE_BROWSED);
  ClientPhishingRequest request;
  request.set_url("http://www.foo.com/bar.html");
  request.set_client_score(0.5);
  StartExtractFeatures(&request);

  ClientPhishingRequest request2;
  request2.set_url("http://www.foo.com/goo.html");
  request2.set_client_score(1.0);
  StartExtractFeatures(&request2);

  MessageLoop::current()->Run();
  EXPECT_TRUE(success_[&request]);
  // Success is false because the second URL is not in the history and we are
  // not able to distinguish between a missing URL in the history and an error.
  EXPECT_FALSE(success_[&request2]);
}

TEST_F(BrowserFeatureExtractorTest, BrowseFeatures) {
  history_service()->AddPage(GURL("http://www.foo.com/"),
                             history::SOURCE_BROWSED);
  ClientPhishingRequest request;
  request.set_url("http://www.foo.com/");
  request.set_client_score(0.5);

  browse_info_->url = GURL("http://www.foo.com/");
  browse_info_->referrer = GURL("http://google.com/");
  browse_info_->transition =
      PageTransition::LINK | PageTransition::FORWARD_BACK;

  EXPECT_TRUE(ExtractFeatures(&request));
  std::map<std::string, double> features;
  GetFeatureMap(request, &features);

  EXPECT_EQ("http://google.com/", request.referrer_url());
  EXPECT_EQ(0.0, features[features::kHasSSLReferrer]);
  EXPECT_EQ(0.0, features[features::kPageTransitionType]);

  request.Clear();
  request.set_url("http://www.foo.com/");
  request.set_client_score(0.5);

  browse_info_->referrer = GURL("https://bankofamerica.com/");
  browse_info_->transition =
      PageTransition::TYPED | PageTransition::FORWARD_BACK;
  browse_info_->ips.insert("193.5.163.8");
  browse_info_->ips.insert("23.94.78.1");

  EXPECT_CALL(*service_, IsBadIpAddress("193.5.163.8")).WillOnce(Return(true));
  EXPECT_CALL(*service_, IsBadIpAddress("23.94.78.1")).WillOnce(Return(false));

  EXPECT_TRUE(ExtractFeatures(&request));
  features.clear();
  GetFeatureMap(request, &features);

  EXPECT_FALSE(request.has_referrer_url());
  EXPECT_EQ(1.0, features[features::kHasSSLReferrer]);
  EXPECT_EQ(1.0, features[features::kPageTransitionType]);
  EXPECT_EQ(1.0, features[std::string(features::kBadIpFetch) + "193.5.163.8"]);
  EXPECT_FALSE(features.count(std::string(features::kBadIpFetch) +
                              "23.94.78.1"));
}
}  // namespace safe_browsing
