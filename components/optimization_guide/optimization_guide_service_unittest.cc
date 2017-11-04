// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/optimization_guide_service.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/version.h"
#include "components/optimization_guide/notification_types.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_source.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace optimization_guide {

const base::FilePath::CharType kFileName[] = FILE_PATH_LITERAL("somefile.pb");

class TestNotificationObserver : public content::NotificationObserver {
 public:
  TestNotificationObserver(OptimizationGuideService* optimization_guide_service)
      : optimization_guide_service_(optimization_guide_service),
        received_notification_(false) {
    registrar_.Add(
        this, NOTIFICATION_HINTS_READY,
        content::Source<OptimizationGuideService>(optimization_guide_service_));
  }

  ~TestNotificationObserver() override {}

  void Observe(int notification_type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    ASSERT_EQ(notification_type, NOTIFICATION_HINTS_READY);
    ASSERT_EQ(optimization_guide_service_,
              content::Source<OptimizationGuideService>(source).ptr());
    received_notification_ = true;
    received_config_.CopyFrom(*content::Details<Configuration>(details).ptr());
  }

  bool received_notification() { return received_notification_; }

  const Configuration& received_config() { return received_config_; }

 private:
  OptimizationGuideService* optimization_guide_service_;
  bool received_notification_;
  Configuration received_config_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(TestNotificationObserver);
};

class OptimizationGuideServiceTest : public testing::Test {
 public:
  OptimizationGuideServiceTest() {}

  ~OptimizationGuideServiceTest() override {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    optimization_guide_service_ = base::MakeUnique<OptimizationGuideService>(
        scoped_task_environment_.GetMainThreadTaskRunner());

    observer_ = base::MakeUnique<TestNotificationObserver>(
        optimization_guide_service_.get());
  }

  OptimizationGuideService* optimization_guide_service() {
    return optimization_guide_service_.get();
  }

  TestNotificationObserver* observer() { return observer_.get(); }

  void UpdateHints(base::Version version, base::FilePath filePath) {
    UnindexedHintsInfo info(version, filePath);
    optimization_guide_service_->ReadAndIndexHints(info);
  }

  void WriteConfigToFile(base::FilePath filePath, const Configuration& config) {
    std::string serialized_config;
    ASSERT_TRUE(config.SerializeToString(&serialized_config));
    ASSERT_EQ(static_cast<int32_t>(serialized_config.length()),
              base::WriteFile(filePath, serialized_config.data(),
                              serialized_config.length()));
  }

  base::FilePath temp_dir() { return temp_dir_.GetPath(); }

 protected:
  void RunUntilIdle() {
    scoped_task_environment_.RunUntilIdle();
    base::RunLoop().RunUntilIdle();
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::ScopedTempDir temp_dir_;

  std::unique_ptr<OptimizationGuideService> optimization_guide_service_;
  std::unique_ptr<TestNotificationObserver> observer_;

  DISALLOW_COPY_AND_ASSIGN(OptimizationGuideServiceTest);
};

TEST_F(OptimizationGuideServiceTest, InvalidVersionIgnored) {
  UpdateHints(base::Version(""), base::FilePath(kFileName));

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
}

TEST_F(OptimizationGuideServiceTest, PastVersionIgnored) {
  optimization_guide_service()->SetLatestProcessedVersionForTesting(
      base::Version("2.0.0"));

  base::FilePath filePath = temp_dir().Append(kFileName);
  Configuration config;
  Hint* hint = config.add_hints();
  hint->set_key("google.com");
  ASSERT_NO_FATAL_FAILURE(WriteConfigToFile(filePath, config));

  UpdateHints(base::Version("1.0.0"), filePath);

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
}

TEST_F(OptimizationGuideServiceTest, SameVersionIgnored) {
  base::Version version("1.0.0");
  optimization_guide_service()->SetLatestProcessedVersionForTesting(version);

  base::FilePath filePath = temp_dir().Append(kFileName);
  Configuration config;
  Hint* hint = config.add_hints();
  hint->set_key("google.com");
  ASSERT_NO_FATAL_FAILURE(WriteConfigToFile(filePath, config));

  UpdateHints(version, filePath);

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
}

TEST_F(OptimizationGuideServiceTest, EmptyFileNameIgnored) {
  UpdateHints(base::Version("1.0.0"), base::FilePath(FILE_PATH_LITERAL("")));

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
}

TEST_F(OptimizationGuideServiceTest, InvalidFileIgnored) {
  UpdateHints(base::Version("1.0.0"), base::FilePath(kFileName));

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
}

TEST_F(OptimizationGuideServiceTest, NotAConfigInFileIgnored) {
  base::FilePath filePath = temp_dir().Append(kFileName);
  ASSERT_EQ(static_cast<int32_t>(3), base::WriteFile(filePath, "boo", 3));

  UpdateHints(base::Version("1.0.0"), filePath);

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
}

TEST_F(OptimizationGuideServiceTest, HappyCase) {
  base::FilePath filePath = temp_dir().Append(kFileName);
  Configuration config;
  Hint* hint = config.add_hints();
  hint->set_key("google.com");
  ASSERT_NO_FATAL_FAILURE(WriteConfigToFile(filePath, config));

  UpdateHints(base::Version("1.0.0"), filePath);

  RunUntilIdle();

  EXPECT_TRUE(observer()->received_notification());
  const Configuration& received_config = observer()->received_config();
  ASSERT_EQ(1, received_config.hints_size());
  ASSERT_EQ("google.com", received_config.hints()[0].key());
}

}  // namespace optimization_guide
