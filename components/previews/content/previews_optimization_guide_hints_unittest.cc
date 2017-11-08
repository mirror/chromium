// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/content/previews_optimization_guide_hints.h"

#include <memory>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_task_environment.h"
#include "components/optimization_guide/optimization_guide_service.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace previews {

class PreviewsOptimizationGuideHintsTest : public testing::Test {
 public:
  PreviewsOptimizationGuideHintsTest()
      : optimization_guide_service_(
            std::make_unique<optimization_guide::OptimizationGuideService>(
                scoped_task_environment_.GetMainThreadTaskRunner())) {}

  ~PreviewsOptimizationGuideHintsTest() override {}

  void SetUp() override {
    guide_ = std::make_unique<PreviewsOptimizationGuideHints>(
        optimization_guide_service_.get(),
        scoped_task_environment_.GetMainThreadTaskRunner());
  }

  PreviewsOptimizationGuideHints* guide() { return guide_.get(); }

  void UpdateHints(const optimization_guide::proto::Configuration& config) {
    guide_->OnHintsProcessed(config);
  }

  std::unique_ptr<net::URLRequest> CreateRequestWithURL(const GURL& url) const {
    return context_.CreateRequest(url, net::DEFAULT_PRIORITY, nullptr,
                                  TRAFFIC_ANNOTATION_FOR_TESTS);
  }

 protected:
  void RunUntilIdle() {
    scoped_task_environment_.RunUntilIdle();
    base::RunLoop().RunUntilIdle();
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<PreviewsOptimizationGuideHints> guide_;
  std::unique_ptr<optimization_guide::OptimizationGuideService>
      optimization_guide_service_;

  net::TestURLRequestContext context_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsOptimizationGuideHintsTest);
};

TEST_F(PreviewsOptimizationGuideHintsTest, ObserveWhitelistPopulatedCorrectly) {
  optimization_guide::proto::Configuration config;
  optimization_guide::proto::Hint* hint1 = config.add_hints();
  hint1->set_key("facebook.com");
  hint1->set_key_representation(optimization_guide::proto::HOST_SUFFIX);
  optimization_guide::proto::Optimization* optimization1 =
      hint1->add_whitelisted_optimizations();
  optimization1->set_optimization_type(optimization_guide::proto::NOSCRIPT);
  // Add a second optimization to ensure that the applicable optimizations are
  // still whitelisted.
  optimization_guide::proto::Optimization* optimization2 =
      hint1->add_whitelisted_optimizations();
  optimization2->set_optimization_type(
      optimization_guide::proto::TYPE_UNSPECIFIED);
  // Add a second hint.
  optimization_guide::proto::Hint* hint2 = config.add_hints();
  hint2->set_key("twitter.com");
  hint2->set_key_representation(optimization_guide::proto::HOST_SUFFIX);
  optimization_guide::proto::Optimization* optimization3 =
      hint2->add_whitelisted_optimizations();
  optimization3->set_optimization_type(optimization_guide::proto::NOSCRIPT);
  UpdateHints(config);

  RunUntilIdle();

  // Twitter and Facebook should be whitelisted but not Google.
  EXPECT_TRUE(guide()->IsWhitelisted(
      *CreateRequestWithURL(GURL("https://m.facebook.com")),
      PreviewsType::NOSCRIPT));
  EXPECT_TRUE(guide()->IsWhitelisted(
      *CreateRequestWithURL(GURL("https://m.twitter.com/example")),
      PreviewsType::NOSCRIPT));
  EXPECT_FALSE(
      guide()->IsWhitelisted(*CreateRequestWithURL(GURL("https://google.com")),
                             PreviewsType::NOSCRIPT));
}

TEST_F(PreviewsOptimizationGuideHintsTest, ObserveUnsupportedKeyRepIsIgnored) {
  optimization_guide::proto::Configuration config;
  optimization_guide::proto::Hint* hint = config.add_hints();
  hint->set_key("facebook.com");
  hint->set_key_representation(
      optimization_guide::proto::REPRESENTATION_UNSPECIFIED);
  optimization_guide::proto::Optimization* optimization =
      hint->add_whitelisted_optimizations();
  optimization->set_optimization_type(optimization_guide::proto::NOSCRIPT);
  UpdateHints(config);

  RunUntilIdle();

  std::unique_ptr<net::URLRequest> request =
      CreateRequestWithURL(GURL("https://m.facebook.com"));
  EXPECT_FALSE(guide()->IsWhitelisted(*request, PreviewsType::NOSCRIPT));
}

TEST_F(PreviewsOptimizationGuideHintsTest,
       ObserveUnsupportedOptimizationIsIgnored) {
  optimization_guide::proto::Configuration config;
  optimization_guide::proto::Hint* hint = config.add_hints();
  hint->set_key("facebook.com");
  hint->set_key_representation(optimization_guide::proto::HOST_SUFFIX);
  optimization_guide::proto::Optimization* optimization =
      hint->add_whitelisted_optimizations();
  optimization->set_optimization_type(
      optimization_guide::proto::TYPE_UNSPECIFIED);
  UpdateHints(config);

  RunUntilIdle();

  std::unique_ptr<net::URLRequest> request =
      CreateRequestWithURL(GURL("https://m.facebook.com"));
  EXPECT_FALSE(guide()->IsWhitelisted(*request, PreviewsType::NOSCRIPT));
}

TEST_F(PreviewsOptimizationGuideHintsTest, ObserveHintWithNoKeyFailsDcheck) {
  optimization_guide::proto::Configuration config;
  optimization_guide::proto::Hint* hint = config.add_hints();
  hint->set_key_representation(optimization_guide::proto::HOST_SUFFIX);
  optimization_guide::proto::Optimization* optimization =
      hint->add_whitelisted_optimizations();
  optimization->set_optimization_type(optimization_guide::proto::NOSCRIPT);

  EXPECT_DCHECK_DEATH({
    UpdateHints(config);
    RunUntilIdle();
  });
}

TEST_F(PreviewsOptimizationGuideHintsTest,
       ObserveConfigWithDuplicateKeysFailsDcheck) {
  optimization_guide::proto::Configuration config;
  optimization_guide::proto::Hint* hint1 = config.add_hints();
  hint1->set_key("facebook.com");
  hint1->set_key_representation(optimization_guide::proto::HOST_SUFFIX);
  optimization_guide::proto::Optimization* optimization1 =
      hint1->add_whitelisted_optimizations();
  optimization1->set_optimization_type(optimization_guide::proto::NOSCRIPT);
  optimization_guide::proto::Hint* hint2 = config.add_hints();
  hint2->set_key("facebook.com");
  hint2->set_key_representation(optimization_guide::proto::HOST_SUFFIX);
  optimization_guide::proto::Optimization* optimization2 =
      hint2->add_whitelisted_optimizations();
  optimization2->set_optimization_type(optimization_guide::proto::NOSCRIPT);

  EXPECT_DCHECK_DEATH({
    UpdateHints(config);
    RunUntilIdle();
  });
}

// TODO(crbug.com/777892): Add a test for releasing the observer on destruction.

}  // namespace previews
