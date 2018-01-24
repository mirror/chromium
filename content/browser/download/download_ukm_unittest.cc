// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_ukm.h"

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/ukm/test_ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::_;
using UkmDownloadStarted = ukm::builders::Download_Started;
using UkmDownloadInterrupted = ukm::builders::Download_Interrupted;
using UkmDownloadResumed = ukm::builders::Download_Resumed;
using UkmDownloadCompleted = ukm::builders::Download_Completed;

namespace content {

constexpr char kTestOrigin[] = "https://test.google.com/";

class DownloadUkmTest : public testing::Test {
 public:
  DownloadUkmTest() : download_id_(123), url_(GURL(kTestOrigin)) {}
  ~DownloadUkmTest() override = default;

  void SetUp() override {
    download_ukm_helper_ =
        std::make_unique<DownloadUkmHelper>(download_id_, url_);
  }

 protected:
  int download_id_;
  GURL url_;

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<DownloadUkmHelper> download_ukm_helper_;
  ukm::TestAutoSetUkmRecorder test_recorder_;

  DISALLOW_COPY_AND_ASSIGN(DownloadUkmTest);
};

TEST_F(DownloadUkmTest, TestBasicReporting) {
  // RecordDownloadStarted
  int file_type = 2;
  download_ukm_helper_->RecordDownloadStarted(file_type);

  const ukm::UkmSource* source =
      test_recorder_.GetSourceForUrl(GURL(kTestOrigin));
  EXPECT_TRUE(source);

  test_recorder_.ExpectMetric(*source, UkmDownloadStarted::kEntryName,
                              UkmDownloadStarted::kDownloadIdName,
                              download_id_);
  test_recorder_.ExpectMetric(*source, UkmDownloadStarted::kEntryName,
                              UkmDownloadStarted::kFileTypeName, file_type);

  // RecordDownloadInterrupted, has change in file size.
  int change_in_file_size = 1000;
  int reason = 3;
  int resulting_file_size = 2000;
  int time_since_start = 250;
  download_ukm_helper_->RecordDownloadInterrupted(
      change_in_file_size, reason, resulting_file_size,
      base::TimeDelta::FromMilliseconds(time_since_start));

  test_recorder_.ExpectMetric(*source, UkmDownloadInterrupted::kEntryName,
                              UkmDownloadInterrupted::kDownloadIdName,
                              download_id_);
  test_recorder_.ExpectMetric(
      *source, UkmDownloadInterrupted::kEntryName,
      UkmDownloadInterrupted::kChangeInFileSizeName,
      download_ukm_helper_->CalcExponentialBucket(change_in_file_size));
  test_recorder_.ExpectMetric(*source, UkmDownloadInterrupted::kEntryName,
                              UkmDownloadInterrupted::kReasonName, reason);
  test_recorder_.ExpectMetric(
      *source, UkmDownloadInterrupted::kEntryName,
      UkmDownloadInterrupted::kResultingFileSizeName,
      download_ukm_helper_->CalcExponentialBucket(resulting_file_size));

  // RecordDownloadResumed.
  int mode = 4;
  int time_since_start_resume = 300;
  download_ukm_helper_->RecordDownloadResumed(
      mode, base::TimeDelta::FromMilliseconds(time_since_start_resume));

  test_recorder_.ExpectMetric(*source, UkmDownloadResumed::kEntryName,
                              UkmDownloadResumed::kDownloadIdName,
                              download_id_);
  test_recorder_.ExpectMetric(*source, UkmDownloadResumed::kEntryName,
                              UkmDownloadResumed::kModeName, mode);
  test_recorder_.ExpectMetric(*source, UkmDownloadResumed::kEntryName,
                              UkmDownloadResumed::kTimeSinceStartName,
                              time_since_start_resume);

  // RecordDownloadCompleted.
  int resulting_file_size_completed = 3000;
  int time_since_start_completed = 400;
  download_ukm_helper_->RecordDownloadCompleted(
      resulting_file_size_completed,
      base::TimeDelta::FromMilliseconds(time_since_start_completed));

  test_recorder_.ExpectMetric(*source, UkmDownloadCompleted::kEntryName,
                              UkmDownloadCompleted::kDownloadIdName,
                              download_id_);
  test_recorder_.ExpectMetric(*source, UkmDownloadCompleted::kEntryName,
                              UkmDownloadCompleted::kResultingFileSizeName,
                              download_ukm_helper_->CalcExponentialBucket(
                                  resulting_file_size_completed));
  test_recorder_.ExpectMetric(*source, UkmDownloadCompleted::kEntryName,
                              UkmDownloadCompleted::kTimeSinceStartName,
                              time_since_start_completed);
}

}  // namespace content
