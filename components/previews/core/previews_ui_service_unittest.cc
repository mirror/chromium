// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_ui_service.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "components/previews/core/previews_black_list.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_io_data.h"
#include "components/previews/core/previews_logger.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace previews {

namespace {

class TestPreviewsUIService : public PreviewsUIService {
 public:
  TestPreviewsUIService(
      PreviewsIOData* previews_io_data,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      std::unique_ptr<PreviewsOptOutStore> previews_opt_out_store,
      std::unique_ptr<PreviewsLogger> logger)
      : PreviewsUIService(previews_io_data,
                          io_task_runner,
                          std::move(previews_opt_out_store),
                          PreviewsIsEnabledCallback(),
                          std::move(logger)),
        io_data_set_(false) {}
  ~TestPreviewsUIService() override {}

  // Set |io_data_set_| to true and use base class functionality.
  void SetIOData(base::WeakPtr<PreviewsIOData> previews_io_data) override {
    io_data_set_ = true;
    PreviewsUIService::SetIOData(previews_io_data);
  }

  // Whether SetIOData was called.
  bool io_data_set() { return io_data_set_; }

 private:
  // Whether SetIOData was called.
  bool io_data_set_;
};

// Mock class of PreviewsLogger for checking passed in parameters.
class TestPreviewsLogger : public PreviewsLogger {
 public:
  TestPreviewsLogger() {}

  void LogPreviewNavigation(const GURL& url,
                            PreviewsType type,
                            bool opt_out,
                            base::Time time) override {
    url_ = GURL(url);
    type_ = type;
    opt_out_ = opt_out;
    time_ = time;
  }

  void LogPreviewsDecisionMade(PreviewsEligibilityReason reason,
                               const GURL& url,
                               base::Time time,
                               PreviewsType type) override {
    reason_ = reason;
    url_ = GURL(url);
    time_ = time;
    type_ = type;
  }

  // Return the logged PreviewsEligibilityReason.
  PreviewsEligibilityReason reason() { return reason_; }

  // Return the logged url.
  GURL url() const { return url_; }

  // Return the logged type.
  PreviewsType type() const { return type_; }

  // Return the logged opt_out.
  bool opt_out() { return opt_out_; }

  // Return the logged time.
  base::Time time() { return time_; }

 private:
  PreviewsEligibilityReason reason_;
  GURL url_;
  PreviewsType type_;
  bool opt_out_;
  base::Time time_;
};

class PreviewsUIServiceTest : public testing::Test {
 public:
  PreviewsUIServiceTest() {}

  ~PreviewsUIServiceTest() override {}

  void SetUp() override {
    std::unique_ptr<TestPreviewsLogger> logger =
        base::MakeUnique<TestPreviewsLogger>();

    // Use to testing logger data.
    logger_ptr_ = logger.get();

    set_io_data(base::WrapUnique(
        new PreviewsIOData(loop_.task_runner(), loop_.task_runner())));
    set_ui_service(base::WrapUnique(new TestPreviewsUIService(
        io_data(), loop_.task_runner(), nullptr, std::move(logger))));
    base::RunLoop().RunUntilIdle();
  }

  void set_io_data(std::unique_ptr<PreviewsIOData> io_data) {
    io_data_ = std::move(io_data);
  }

  PreviewsIOData* io_data() { return io_data_.get(); }

  void set_ui_service(std::unique_ptr<TestPreviewsUIService> ui_service) {
    ui_service_ = std::move(ui_service);
  }

  TestPreviewsUIService* ui_service() { return ui_service_.get(); }

 protected:
  // Run this test on a single thread.
  base::MessageLoopForIO loop_;
  TestPreviewsLogger* logger_ptr_;

 private:
  std::unique_ptr<PreviewsIOData> io_data_;
  std::unique_ptr<TestPreviewsUIService> ui_service_;
};

}  // namespace

TEST_F(PreviewsUIServiceTest, TestInitialization) {
  // After the outstanding posted tasks have run, SetIOData should have been
  // called for |ui_service_|.
  EXPECT_TRUE(ui_service()->io_data_set());
}

TEST_F(PreviewsUIServiceTest, TestLogPreviewNavigationPassInCorrectParams) {
  const GURL url_a = GURL("http://www.url_a.com/url_a");
  PreviewsType type_a = PreviewsType::LOFI;
  bool opt_out_a = true;
  base::Time time_a = base::Time::Now();

  ui_service()->LogPreviewNavigation(url_a, type_a, opt_out_a, time_a);

  EXPECT_EQ(url_a, logger_ptr_->url());
  EXPECT_EQ(type_a, logger_ptr_->type());
  EXPECT_EQ(opt_out_a, logger_ptr_->opt_out());
  EXPECT_EQ(time_a, logger_ptr_->time());

  const GURL url_b = GURL("http://www.url_b.com/url_b");
  PreviewsType type_b = PreviewsType::OFFLINE;
  bool opt_out_b = false;
  base::Time time_b = base::Time::Now();

  ui_service()->LogPreviewNavigation(url_b, type_b, opt_out_b, time_b);

  EXPECT_EQ(url_b, logger_ptr_->url());
  EXPECT_EQ(type_b, logger_ptr_->type());
  EXPECT_EQ(opt_out_b, logger_ptr_->opt_out());
  EXPECT_EQ(time_b, logger_ptr_->time());
}

TEST_F(PreviewsUIServiceTest, TestLogPreviewsDecisionMadePassesCorrectParams) {
  PreviewsEligibilityReason reason_a =
      PreviewsEligibilityReason::BLACKLIST_UNAVAILABLE;
  const GURL url_a("http://www.url_a.com/url_a");
  const base::Time time_a = base::Time::Now();
  PreviewsType type_a = PreviewsType::OFFLINE;

  ui_service()->LogPreviewsDecisionMade(reason_a, url_a, time_a, type_a);

  EXPECT_EQ(reason_a, logger_ptr_->reason());
  EXPECT_EQ(url_a, logger_ptr_->url());
  EXPECT_EQ(time_a, logger_ptr_->time());
  EXPECT_EQ(type_a, logger_ptr_->type());

  PreviewsEligibilityReason reason_b =
      PreviewsEligibilityReason::NETWORK_NOT_SLOW;
  const GURL url_b("http://www.url_b.com/url_b");
  const base::Time time_b = base::Time::Now();
  PreviewsType type_b = PreviewsType::LOFI;

  ui_service()->LogPreviewsDecisionMade(reason_b, url_b, time_b, type_b);

  EXPECT_EQ(reason_b, logger_ptr_->reason());
  EXPECT_EQ(url_b, logger_ptr_->url());
  EXPECT_EQ(type_b, logger_ptr_->type());
  EXPECT_EQ(time_b, logger_ptr_->time());
}

}  // namespace previews
