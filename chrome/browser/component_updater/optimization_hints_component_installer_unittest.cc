// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/optimization_hints_component_installer.h"

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/version.h"
#include "chrome/browser/optimization_guide/optimization_guide_constants.h"
#include "chrome/browser/optimization_guide/optimization_guide_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

static const base::Version& kTestHintsVersion = base::Version("1.2.3");

class TestOptimizationGuideService
    : public optimization_guide::OptimizationGuideService {
 public:
  TestOptimizationGuideService(
      const scoped_refptr<base::SequencedTaskRunner> background_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner)
      : optimization_guide::OptimizationGuideService(background_task_runner,
                                                     io_thread_task_runner) {}
  ~TestOptimizationGuideService() override {}

  using UnindexedHintsInfo = optimization_guide::UnindexedHintsInfo;
  void ReadAndIndexHints(
      const UnindexedHintsInfo& unindexed_hints_info) override {
    unindexed_hints_info_ =
        base::MakeUnique<UnindexedHintsInfo>(unindexed_hints_info);
  }

  UnindexedHintsInfo* unindexed_hints_info() const {
    return unindexed_hints_info_.get();
  }

 private:
  std::unique_ptr<UnindexedHintsInfo> unindexed_hints_info_;

  DISALLOW_COPY_AND_ASSIGN(TestOptimizationGuideService);
};

class OptimizationHintsMockComponentUpdateService
    : public component_updater::MockComponentUpdateService {
 public:
  OptimizationHintsMockComponentUpdateService() {}
  ~OptimizationHintsMockComponentUpdateService() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(OptimizationHintsMockComponentUpdateService);
};

}  // namespace

namespace component_updater {

class OptimizationHintsComponentInstallerTest : public PlatformTest {
 public:
  OptimizationHintsComponentInstallerTest() {}

  void SetUp() override {
    PlatformTest::SetUp();

    ASSERT_TRUE(component_install_dir_.CreateUniqueTempDir());

    auto optimization_guide_service =
        base::MakeUnique<TestOptimizationGuideService>(
            base::ThreadTaskRunnerHandle::Get(),
            base::ThreadTaskRunnerHandle::Get());
    optimization_guide_service_ = optimization_guide_service.get();

    pref_service_ = base::MakeUnique<TestingPrefServiceSimple>();

    TestingBrowserProcess::GetGlobal()->SetOptimizationGuideService(
        std::move(optimization_guide_service));
    policy_ = base::MakeUnique<OptimizationHintsComponentInstallerPolicy>();
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetOptimizationGuideService(nullptr);
    PlatformTest::TearDown();
  }

  TestOptimizationGuideService* service() {
    return optimization_guide_service_;
  }

  base::FilePath component_install_dir() {
    return component_install_dir_.GetPath();
  }

  TestingPrefServiceSimple* profile_prefs() { return pref_service_.get(); }

  void EnableOptimizationHintsFeature(bool enabled) {
    std::unique_ptr<base::FeatureList> feature_list =
        base::MakeUnique<base::FeatureList>();
    if (enabled) {
      // The feature is explicitly enabled on the command-line.
      feature_list->InitializeFromCommandLine("OptimizationHints", "");
    }
    base::FeatureList::ClearInstanceForTesting();
    base::FeatureList::SetInstance(std::move(feature_list));
  }

  void CreateTestOptimizationHints(const std::string& hints_content) {
    base::FilePath hints_path = component_install_dir().Append(
        optimization_guide::kUnindexedHintsFileName);
    ASSERT_EQ(static_cast<int32_t>(hints_content.length()),
              base::WriteFile(hints_path, hints_content.data(),
                              hints_content.length()));
  }

  void LoadOptimizationHints(const base::Version& ruleset_format) {
    std::unique_ptr<base::DictionaryValue> manifest(new base::DictionaryValue);
    if (ruleset_format.IsValid()) {
      manifest->SetString(
          OptimizationHintsComponentInstallerPolicy::kManifestRulesetFormatKey,
          ruleset_format.GetString());
    }
    ASSERT_TRUE(
        policy_->VerifyInstallation(*manifest, component_install_dir()));
    const base::Version expected_version(kTestHintsVersion);
    policy_->ComponentReady(expected_version, component_install_dir(),
                            std::move(manifest));
    base::RunLoop().RunUntilIdle();
  }

 protected:
  void RunUntilIdle() {
    scoped_task_environment_.RunUntilIdle();
    base::RunLoop().RunUntilIdle();
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  base::ScopedTempDir component_install_dir_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;

  std::unique_ptr<OptimizationHintsComponentInstallerPolicy> policy_;

  TestOptimizationGuideService* optimization_guide_service_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(OptimizationHintsComponentInstallerTest);
};

TEST_F(OptimizationHintsComponentInstallerTest,
       ComponentRegistrationWhenFeatureDisabled) {
  EnableOptimizationHintsFeature(false);
  std::unique_ptr<OptimizationHintsMockComponentUpdateService> cus(
      new OptimizationHintsMockComponentUpdateService());
  EXPECT_CALL(*cus, RegisterComponent(testing::_)).Times(0);
  RegisterOptimizationHintsComponent(cus.get(), profile_prefs());
  RunUntilIdle();
}

TEST_F(OptimizationHintsComponentInstallerTest,
       ComponentRegistrationWhenFeatureEnabledButDataSaverDisabled) {
  TestingPrefServiceSimple* prefs = profile_prefs();
  prefs->registry()->RegisterBooleanPref(prefs::kDataSaverEnabled, false);
  EnableOptimizationHintsFeature(true);
  std::unique_ptr<OptimizationHintsMockComponentUpdateService> cus(
      new OptimizationHintsMockComponentUpdateService());
  EXPECT_CALL(*cus, RegisterComponent(testing::_)).Times(0);
  RegisterOptimizationHintsComponent(cus.get(), prefs);
  RunUntilIdle();
}

TEST_F(OptimizationHintsComponentInstallerTest,
       ComponentRegistrationWhenFeatureEnabledButNoProfilePrefs) {
  EnableOptimizationHintsFeature(true);
  std::unique_ptr<OptimizationHintsMockComponentUpdateService> cus(
      new OptimizationHintsMockComponentUpdateService());
  EXPECT_CALL(*cus, RegisterComponent(testing::_)).Times(0);
  RegisterOptimizationHintsComponent(cus.get(), nullptr);
  RunUntilIdle();
}

TEST_F(OptimizationHintsComponentInstallerTest,
       ComponentRegistrationWhenFeatureEnabledAndDataSaverEnabled) {
  TestingPrefServiceSimple* prefs = profile_prefs();
  prefs->registry()->RegisterBooleanPref(prefs::kDataSaverEnabled, true);
  EnableOptimizationHintsFeature(true);
  std::unique_ptr<OptimizationHintsMockComponentUpdateService> cus(
      new OptimizationHintsMockComponentUpdateService());
  EXPECT_CALL(*cus, RegisterComponent(testing::_))
      .Times(1)
      .WillOnce(testing::Return(true));
  RegisterOptimizationHintsComponent(cus.get(), profile_prefs());
  RunUntilIdle();
}

TEST_F(OptimizationHintsComponentInstallerTest, NoRulesetFormatIgnored) {
  ASSERT_TRUE(service());
  ASSERT_NO_FATAL_FAILURE(CreateTestOptimizationHints("some hints"));

  ASSERT_NO_FATAL_FAILURE(LoadOptimizationHints(base::Version("")));
  EXPECT_EQ(nullptr, service()->unindexed_hints_info());
}

TEST_F(OptimizationHintsComponentInstallerTest, FutureRulesetFormatIgnored) {
  ASSERT_TRUE(service());
  ASSERT_NO_FATAL_FAILURE(CreateTestOptimizationHints("some hints"));
  auto& ruleset_components =
      OptimizationHintsComponentInstallerPolicy::kCurrentRulesetFormat
          .components();
  const std::vector<uint32_t> future_ruleset_components = {
      ruleset_components[0] + 1, ruleset_components[1], ruleset_components[2]};

  ASSERT_NO_FATAL_FAILURE(
      LoadOptimizationHints(base::Version(future_ruleset_components)));
  EXPECT_EQ(nullptr, service()->unindexed_hints_info());
}

TEST_F(OptimizationHintsComponentInstallerTest, LoadFileWithData) {
  ASSERT_TRUE(service());

  const std::string expected_hints = "some hints";
  ASSERT_NO_FATAL_FAILURE(CreateTestOptimizationHints(expected_hints));
  ASSERT_NO_FATAL_FAILURE(LoadOptimizationHints(
      OptimizationHintsComponentInstallerPolicy::kCurrentRulesetFormat));

  auto* unindexed_hints_info = service()->unindexed_hints_info();
  EXPECT_NE(nullptr, unindexed_hints_info);
  EXPECT_EQ(kTestHintsVersion, unindexed_hints_info->hints_version);
  std::string actual_hints;
  ASSERT_TRUE(
      base::ReadFileToString(unindexed_hints_info->hints_path, &actual_hints));
  EXPECT_EQ(expected_hints, actual_hints);
}

}  // namespace component_updater
