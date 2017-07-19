// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/active_users_logger.h"

#include "base/memory/ptr_util.h"
#include "base/test/histogram_tester.h"
#include "chromeos/components/tether/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

class ActiveUsersLoggerTest : public testing::Test {
 protected:
  ActiveUsersLoggerTest() {}

  void SetUp() override {
    test_pref_service_ = base::MakeUnique<TestingPrefServiceSimple>();
    ActiveUsersLogger::RegisterPrefs(test_pref_service_->registry());

    active_users_logger_ =
        base::MakeUnique<ActiveUsersLogger>(test_pref_service_.get());
  }

  void SetLastUserActiveTime(int num_days_ago) {
    base::Time now = base::Time::Now();
    base::Time last_active_time = now - base::TimeDelta::FromDays(num_days_ago);
    test_pref_service_->SetInt64(prefs::kUserLastActiveTimestamp,
                                 last_active_time.ToInternalValue());
  }

  void Verify1DA(bool is_expected) {
    VerifyNDAHistogram("InstantTethering.ActiveUsers.1DA", is_expected);
  }

  void Verify7DA(bool is_expected) {
    VerifyNDAHistogram("InstantTethering.ActiveUsers.7DA", is_expected);
  }

  void Verify14DA(bool is_expected) {
    VerifyNDAHistogram("InstantTethering.ActiveUsers.14DA", is_expected);
  }

  void Verify28DA(bool is_expected) {
    VerifyNDAHistogram("InstantTethering.ActiveUsers.28DA", is_expected);
  }

  void VerifyNDAHistogram(const std::string& histogram_name, bool is_expected) {
    if (is_expected) {
      histogram_tester_.ExpectUniqueSample(
          histogram_name, ActiveUsersLogger::ActiveUser::ACTIVE, 1);
    } else {
      histogram_tester_.ExpectTotalCount(histogram_name, 0);
    }
  }

  std::unique_ptr<TestingPrefServiceSimple> test_pref_service_;

  std::unique_ptr<ActiveUsersLogger> active_users_logger_;

  base::HistogramTester histogram_tester_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ActiveUsersLoggerTest);
};

TEST_F(ActiveUsersLoggerTest, RecordUserWasActive) {
  active_users_logger_->RecordUserWasActive();

  active_users_logger_->ReportUserActivity();

  Verify1DA(true);
  Verify7DA(true);
  Verify14DA(true);
  Verify28DA(true);
}

TEST_F(ActiveUsersLoggerTest, TestToday) {
  SetLastUserActiveTime(0);

  active_users_logger_->ReportUserActivity();

  Verify1DA(true);
  Verify7DA(true);
  Verify14DA(true);
  Verify28DA(true);
}

TEST_F(ActiveUsersLoggerTest, Test1DayAgo) {
  SetLastUserActiveTime(1);

  active_users_logger_->ReportUserActivity();

  Verify1DA(true);
  Verify7DA(true);
  Verify14DA(true);
  Verify28DA(true);
}

TEST_F(ActiveUsersLoggerTest, Test2DaysAgo) {
  SetLastUserActiveTime(2);

  active_users_logger_->ReportUserActivity();

  Verify1DA(false);
  Verify7DA(true);
  Verify14DA(true);
  Verify28DA(true);
}

TEST_F(ActiveUsersLoggerTest, Test7DaysAgo) {
  SetLastUserActiveTime(7);

  active_users_logger_->ReportUserActivity();

  Verify1DA(false);
  Verify7DA(true);
  Verify14DA(true);
  Verify28DA(true);
}

TEST_F(ActiveUsersLoggerTest, Test8DaysAgo) {
  SetLastUserActiveTime(8);

  active_users_logger_->ReportUserActivity();

  Verify1DA(false);
  Verify7DA(false);
  Verify14DA(true);
  Verify28DA(true);
}

TEST_F(ActiveUsersLoggerTest, Test14DaysAgo) {
  SetLastUserActiveTime(14);

  active_users_logger_->ReportUserActivity();

  Verify1DA(false);
  Verify7DA(false);
  Verify14DA(true);
  Verify28DA(true);
}

TEST_F(ActiveUsersLoggerTest, Test15DaysAgo) {
  SetLastUserActiveTime(15);

  active_users_logger_->ReportUserActivity();

  Verify1DA(false);
  Verify7DA(false);
  Verify14DA(false);
  Verify28DA(true);
}

TEST_F(ActiveUsersLoggerTest, Test28DaysAgo) {
  SetLastUserActiveTime(28);

  active_users_logger_->ReportUserActivity();

  Verify1DA(false);
  Verify7DA(false);
  Verify14DA(false);
  Verify28DA(true);
}

TEST_F(ActiveUsersLoggerTest, Test29DaysAgo) {
  SetLastUserActiveTime(29);

  active_users_logger_->ReportUserActivity();

  Verify1DA(false);
  Verify7DA(false);
  Verify14DA(false);
  Verify28DA(false);
}

}  // namespace tether

}  // namespace chromeos
