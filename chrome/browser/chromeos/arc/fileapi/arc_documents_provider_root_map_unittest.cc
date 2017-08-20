// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root_map.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_util.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_file_system_operation_runner.h"
#include "chrome/test/base/testing_profile.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/common/file_system.mojom.h"
#include "components/arc/test/fake_file_system_instance.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/browser/fileapi/watcher_manager.h"
#include "storage/common/fileapi/directory_entry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace arc {

namespace {

std::vector<FakeFileSystemInstance::Root> makeRoots() {
  std::vector<FakeFileSystemInstance::Root> roots;
  roots.push_back(FakeFileSystemInstance::Root(
      "org.chromium.test", "root_dir", "root_id", "root title",
      "this is a root.", std::vector<uint8_t>{0, 1, 2, 3}, 0xDEADBEEF));
  return roots;
}

void ExpectMatchesSpec(const FakeFileSystemInstance::Root& spec,
                       arc::ArcDocumentsProviderRoot* root) {
  EXPECT_EQ(spec.authority, root->authority());
  EXPECT_EQ(spec.root_document_id, root->root_document_id());
  EXPECT_EQ(spec.id, root->id());
  EXPECT_EQ(spec.title, root->title());
  EXPECT_EQ(spec.summary, root->summary());
  EXPECT_EQ(spec.icon_data, root->icon_data());
}

std::unique_ptr<KeyedService> CreateFileSystemOperationRunnerForTesting(
    content::BrowserContext* context) {
  return ArcFileSystemOperationRunner::CreateForTesting(
      context, ArcServiceManager::Get()->arc_bridge_service());
}

class ArcDocumentsProviderRootMapTest : public testing::Test {
 public:
  ArcDocumentsProviderRootMapTest() = default;
  ~ArcDocumentsProviderRootMapTest() override = default;

  void SetUp() override {
    arc_service_manager_ = base::MakeUnique<ArcServiceManager>();
    profile_ = base::MakeUnique<TestingProfile>();
    arc_service_manager_->set_browser_context(profile_.get());
    ArcFileSystemOperationRunner::GetFactory()->SetTestingFactoryAndUse(
        profile_.get(), &CreateFileSystemOperationRunnerForTesting);
    arc_service_manager_->arc_bridge_service()->file_system()->SetInstance(
        &fake_file_system_);
    roots_ = makeRoots();
    fake_file_system_.SetRoots(roots_);

    // Run the message loop until FileSystemInstance::Init() is called.
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(fake_file_system_.InitCalled());

    map_ = ArcDocumentsProviderRootMap::GetForBrowserContext(profile_.get());
  }

  void TearDown() override {
    // Run all pending tasks before destroying testing profile.
    base::RunLoop().RunUntilIdle();
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  FakeFileSystemInstance fake_file_system_;

  // Use the same initialization/destruction order as
  // ChromeBrowserMainPartsChromeos.
  std::unique_ptr<ArcServiceManager> arc_service_manager_;
  std::unique_ptr<TestingProfile> profile_;
  ArcDocumentsProviderRootMap* map_;
  std::vector<FakeFileSystemInstance::Root> roots_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcDocumentsProviderRootMapTest);
};

}  // namespace

TEST_F(ArcDocumentsProviderRootMapTest, Refresh) {
  base::RunLoop run_loop;
  map_->Refresh(base::Bind(
      [](base::RunLoop* run_loop,
         const std::vector<FakeFileSystemInstance::Root>& rootsSpec,
         std::vector<ArcDocumentsProviderRoot*> roots) {
        run_loop->Quit();
        EXPECT_EQ(1u, roots.size());
        ArcDocumentsProviderRoot* const root = roots.at(0);
        ExpectMatchesSpec(rootsSpec.at(0), root);
      },
      &run_loop, roots_));
  run_loop.Run();
}

}  // namespace arc
