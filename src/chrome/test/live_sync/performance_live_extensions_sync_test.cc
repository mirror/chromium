// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/stringprintf.h"
#include "chrome/browser/sync/profile_sync_service_harness.h"
#include "chrome/test/live_sync/live_extensions_sync_test.h"
#include "chrome/test/live_sync/live_sync_timing_helper.h"

// TODO(braffert): Replicate these tests for apps.

static const int kNumExtensions = 150;

// TODO(braffert): Consider the range / resolution of these test points.
static const int kNumBenchmarkPoints = 18;
static const int kBenchmarkPoints[] = {1, 10, 20, 30, 40, 50, 75, 100, 125,
                                       150, 175, 200, 225, 250, 300, 350, 400,
                                       500};

// TODO(braffert): Move this class into its own .h/.cc files.  What should the
// class files be named as opposed to the file containing the tests themselves?
class PerformanceLiveExtensionsSyncTest
    : public TwoClientLiveExtensionsSyncTest {
 public:
  PerformanceLiveExtensionsSyncTest() : extension_number_(0) {}

  // Adds |num_extensions| new unique extensions to |profile|.
  void AddExtensions(int profile, int num_extensions);

  // Updates the enabled/disabled state for all extensions in |profile|.
  void UpdateExtensions(int profile);

  // Uninstalls all currently installed extensions from |profile|.
  void RemoveExtensions(int profile);

  // Uninstalls all extensions from all profiles.  Called between benchmark
  // iterations.
  void Cleanup();

 private:
  int extension_number_;
  DISALLOW_COPY_AND_ASSIGN(PerformanceLiveExtensionsSyncTest);
};

void PerformanceLiveExtensionsSyncTest::AddExtensions(int profile,
                                                      int num_extensions) {
  for (int i = 0; i < num_extensions; ++i) {
    InstallExtension(GetProfile(profile), extension_number_++);
  }
}

void PerformanceLiveExtensionsSyncTest::UpdateExtensions(int profile) {
  std::vector<int> extensions = GetInstalledExtensions(GetProfile(profile));
  for (std::vector<int>::iterator it = extensions.begin();
       it != extensions.end(); ++it) {
    if (IsExtensionEnabled(GetProfile(profile), *it)) {
      DisableExtension(GetProfile(profile), *it);
    } else {
      EnableExtension(GetProfile(profile), *it);
    }
  }
}

void PerformanceLiveExtensionsSyncTest::RemoveExtensions(int profile) {
  std::vector<int> extensions = GetInstalledExtensions(GetProfile(profile));
  for (std::vector<int>::iterator it = extensions.begin();
       it != extensions.end(); ++it) {
    UninstallExtension(GetProfile(profile), *it);
  }
}

void PerformanceLiveExtensionsSyncTest::Cleanup() {
  for (int i = 0; i < num_clients(); ++i) {
    RemoveExtensions(i);
  }
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AllProfilesHaveSameExtensions());
}

// TCM ID - 7563874.
IN_PROC_BROWSER_TEST_F(PerformanceLiveExtensionsSyncTest, Add) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddExtensions(0, kNumExtensions);
  base::TimeDelta dt =
      LiveSyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
  InstallExtensionsPendingForSync(GetProfile(1));
  ASSERT_TRUE(AllProfilesHaveSameExtensions());

  // TODO(braffert): Compare timings against some target value.
  VLOG(0) << std::endl << "dt: " << dt.InSecondsF() << " s";
}

// TCM ID - 7655397.
IN_PROC_BROWSER_TEST_F(PerformanceLiveExtensionsSyncTest, Update) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddExtensions(0, kNumExtensions);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  InstallExtensionsPendingForSync(GetProfile(1));

  UpdateExtensions(0);
  base::TimeDelta dt =
      LiveSyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_TRUE(AllProfilesHaveSameExtensions());

  // TODO(braffert): Compare timings against some target value.
  VLOG(0) << std::endl << "dt: " << dt.InSecondsF() << " s";
}

// TCM ID - 7567721.
IN_PROC_BROWSER_TEST_F(PerformanceLiveExtensionsSyncTest, Delete) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AddExtensions(0, kNumExtensions);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  InstallExtensionsPendingForSync(GetProfile(1));

  RemoveExtensions(0);
  base::TimeDelta dt =
      LiveSyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_TRUE(AllProfilesHaveSameExtensions());

  // TODO(braffert): Compare timings against some target value.
  VLOG(0) << std::endl << "dt: " << dt.InSecondsF() << " s";
}

IN_PROC_BROWSER_TEST_F(PerformanceLiveExtensionsSyncTest, DISABLED_Benchmark) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  for (int i = 0; i < kNumBenchmarkPoints; ++i) {
    int num_extensions = kBenchmarkPoints[i];
    AddExtensions(0, num_extensions);
    base::TimeDelta dt_add =
        LiveSyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
    InstallExtensionsPendingForSync(GetProfile(1));
    VLOG(0) << std::endl << "Add: " << num_extensions << " "
            << dt_add.InSecondsF();

    UpdateExtensions(0);
    base::TimeDelta dt_update =
        LiveSyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
    VLOG(0) << std::endl << "Update: " << num_extensions << " "
            << dt_update.InSecondsF();

    RemoveExtensions(0);
    base::TimeDelta dt_delete =
        LiveSyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
    VLOG(0) << std::endl << "Delete: " << num_extensions << " "
            << dt_delete.InSecondsF();

    Cleanup();
  }
}
