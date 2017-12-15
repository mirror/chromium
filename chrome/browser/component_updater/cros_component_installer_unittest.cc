// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/cros_component_installer.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process_platform_part_chromeos.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "content/public/browser/browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace component_updater {

class CrOSMockComponentUpdateService
    : public component_updater::MockComponentUpdateService {
 public:
  CrOSMockComponentUpdateService() {}
  ~CrOSMockComponentUpdateService() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CrOSMockComponentUpdateService);
};

class MockCrOSComponent : public CrOSComponent {
 public:
  // gmock does not mock OnceCallback (moved object). So we override
  // InstallComponent and mock its content.
  void Install(
      ComponentUpdateService* cus,
      const std::string& name,
      base::OnceCallback<void(const std::string&)> load_callback) override {
    DoInstall(cus, name, &load_callback);
  }
  MOCK_METHOD3(
      DoInstall,
      void(ComponentUpdateService* cus,
           const std::string& name,
           base::OnceCallback<void(const std::string&)>* load_callback));
  void LoadInternal(
      const std::string& name,
      base::OnceCallback<void(const std::string&)> load_callback) override {
    DoLoadInternal(name, &load_callback);
  }
  MOCK_METHOD2(
      DoLoadInternal,
      void(const std::string& name,
           base::OnceCallback<void(const std::string&)>* load_callback));
};

class CrOSComponentInstallerTest : public PlatformTest {
 public:
  CrOSComponentInstallerTest() {}
  void SetUp() override {
    PlatformTest::SetUp();
    cros_component_ = base::MakeUnique<component_updater::MockCrOSComponent>();
  }

 protected:
  void RunUntilIdle() {
    scoped_task_environment_.RunUntilIdle();
    base::RunLoop().RunUntilIdle();
  }
  MockCrOSComponent* cros_component() { return cros_component_.get(); }

 private:
  std::unique_ptr<component_updater::MockCrOSComponent> cros_component_;

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  DISALLOW_COPY_AND_ASSIGN(CrOSComponentInstallerTest);
};

class MockCrOSComponentInstallerPolicy : public CrOSComponentInstallerPolicy {
 public:
  explicit MockCrOSComponentInstallerPolicy(const ComponentConfig& config)
      : CrOSComponentInstallerPolicy(config) {}
  MOCK_METHOD2(IsCompatible,
               bool(const std::string& env_version_str,
                    const std::string& min_env_version_str));
};

void load_callback(const std::string& result) {}

TEST_F(CrOSComponentInstallerTest, BPPPCompatibleCrOSComponent) {
  const std::string kComponent = "a";
  EXPECT_FALSE(cros_component()->IsCompatible(kComponent));
  EXPECT_EQ(cros_component()->GetCompatiblePath(kComponent).value(),
            std::string());

  const base::FilePath kPath("/component/path/v0");
  cros_component()->RegisterCompatiblePath(kComponent, kPath);
  EXPECT_TRUE(cros_component()->IsCompatible(kComponent));
  EXPECT_EQ(cros_component()->GetCompatiblePath(kComponent), kPath);
  cros_component()->UnregisterCompatiblePath(kComponent);
  EXPECT_FALSE(cros_component()->IsCompatible(kComponent));
}

TEST_F(CrOSComponentInstallerTest, CompatibilityOK) {
  ComponentConfig config("a", "2.1", "");
  MockCrOSComponentInstallerPolicy policy(config);
  EXPECT_CALL(policy, IsCompatible(testing::_, testing::_)).Times(1);
  base::Version version;
  base::FilePath path;
  std::unique_ptr<base::DictionaryValue> manifest =
      base::MakeUnique<base::DictionaryValue>();
  manifest->SetString("min_env_version", "2.1");
  policy.ComponentReady(version, path, std::move(manifest));
}

TEST_F(CrOSComponentInstallerTest, CompatibilityMissingManifest) {
  ComponentConfig config("a", "2.1", "");
  MockCrOSComponentInstallerPolicy policy(config);
  EXPECT_CALL(policy, IsCompatible(testing::_, testing::_)).Times(0);
  base::Version version;
  base::FilePath path;
  std::unique_ptr<base::DictionaryValue> manifest =
      base::MakeUnique<base::DictionaryValue>();
  policy.ComponentReady(version, path, std::move(manifest));
}

TEST_F(CrOSComponentInstallerTest, IsCompatibleOrNot) {
  ComponentConfig config("", "", "");
  CrOSComponentInstallerPolicy policy(config);
  EXPECT_TRUE(policy.IsCompatible("1.0", "1.0"));
  EXPECT_TRUE(policy.IsCompatible("1.1", "1.0"));
  EXPECT_FALSE(policy.IsCompatible("1.0", "1.1"));
  EXPECT_FALSE(policy.IsCompatible("1.0", "2.0"));
  EXPECT_FALSE(policy.IsCompatible("1.c", "1.c"));
  EXPECT_FALSE(policy.IsCompatible("1", "1.1"));
  EXPECT_TRUE(policy.IsCompatible("1.1.1", "1.1"));
}

TEST_F(CrOSComponentInstallerTest, RegisterComponent) {
  std::unique_ptr<CrOSMockComponentUpdateService> cus(
      new CrOSMockComponentUpdateService());
  ComponentConfig config(
      "star-cups-driver", "1.1",
      "6d24de30f671da5aee6d463d9e446cafe9ddac672800a9defe86877dcde6c466");
  EXPECT_CALL(*cus, RegisterComponent(testing::_)).Times(1);
  cros_component()->Register(cus.get(), config, base::OnceClosure());
  RunUntilIdle();
}

TEST_F(CrOSComponentInstallerTest, LoadComponentDownload) {
  EXPECT_CALL(*cros_component(), DoInstall(testing::_, testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(*cros_component(), DoLoadInternal(testing::_, testing::_))
      .Times(0);
  cros_component()->Load("a-fake-component", base::BindOnce(load_callback));
}

TEST_F(CrOSComponentInstallerTest, LoadComponentNodownload) {
  EXPECT_CALL(*cros_component(), DoInstall(testing::_, testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(*cros_component(), DoLoadInternal(testing::_, testing::_))
      .Times(1);
  cros_component()->RegisterCompatiblePath("a-fake-component",
                                           base::FilePath("/some/false/path"));
  cros_component()->Load("a-fake-component", base::BindOnce(load_callback));
}

}  // namespace component_updater
