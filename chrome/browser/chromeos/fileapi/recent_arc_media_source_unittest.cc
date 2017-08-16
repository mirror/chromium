// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_arc_media_source.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_file_system_operation_runner.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/test/base/testing_profile.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/common/file_system.mojom.h"
#include "components/arc/test/fake_file_system_instance.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace {

const char kMediaDocumentsProviderAuthority[] =
    "com.android.providers.media.documents";
const char kImagesRootId[] = "images_root";

std::unique_ptr<KeyedService> CreateFileSystemOperationRunnerForTesting(
    content::BrowserContext* context) {
  return arc::ArcFileSystemOperationRunner::CreateForTesting(
      context, arc::ArcServiceManager::Get()->arc_bridge_service());
}

arc::FakeFileSystemInstance::Document MakeDocument(
    const std::string& display_name,
    const std::string& mime_type) {
  return arc::FakeFileSystemInstance::Document(
      kMediaDocumentsProviderAuthority,  // authority
      display_name,                      // document_id
      "",                                // parent_document_id
      display_name,                      // display_name
      mime_type,                         // mime_type
      0,                                 // size
      0);                                // last_modified
}

class RecentArcMediaSourceTest : public testing::Test {
 public:
  RecentArcMediaSourceTest() = default;

  void SetUp() override {
    arc_service_manager_ = base::MakeUnique<arc::ArcServiceManager>();
    profile_ = base::MakeUnique<TestingProfile>();
    arc_service_manager_->set_browser_context(profile_.get());
    arc::ArcFileSystemOperationRunner::GetFactory()->SetTestingFactoryAndUse(
        profile_.get(), &CreateFileSystemOperationRunnerForTesting);
    arc_service_manager_->arc_bridge_service()->file_system()->SetInstance(
        &fake_file_system_);

    // FIXME: This assertion fails!!!
    ASSERT_TRUE(arc::IsArcAllowedForProfile(profile_.get()));

    source_ = base::MakeUnique<RecentArcMediaSource>(profile_.get());
  }

 protected:
  std::vector<storage::FileSystemURL> GetRecentFiles() {
    std::vector<storage::FileSystemURL> files;

    base::RunLoop run_loop;

    source_->GetRecentFiles(
        RecentContext(nullptr /* file_system_context */, GURL() /* origin */),
        base::BindOnce(
            [](base::RunLoop* run_loop,
               std::vector<storage::FileSystemURL>* out_files,
               std::vector<storage::FileSystemURL> files) {
              run_loop->Quit();
              *out_files = std::move(files);
            },
            &run_loop, &files));

    run_loop.Run();

    return files;
  }

  content::TestBrowserThreadBundle thread_bundle_;
  arc::FakeFileSystemInstance fake_file_system_;

  // Use the same initialization/destruction order as
  // ChromeBrowserMainPartsChromeos.
  std::unique_ptr<arc::ArcServiceManager> arc_service_manager_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<RecentArcMediaSource> source_;
};

TEST_F(RecentArcMediaSourceTest, GetRecentFiles) {
  fake_file_system_.AddRecentDocument(kImagesRootId,
                                      MakeDocument("kitten.png", "image/png"));
  fake_file_system_.AddRecentDocument(kImagesRootId,
                                      MakeDocument("puppy.jpg", "image/jpeg"));

  std::vector<storage::FileSystemURL> files = GetRecentFiles();

  ASSERT_EQ(2u, files.size());
  EXPECT_EQ("com.android.providers.media.documents/images_root/kitten.jpg",
            files[0].path().value());
  EXPECT_EQ("com.android.providers.media.documents/images_root/puppy.jpg",
            files[1].path().value());
}

}  // namespace
}  // namespace chromeos
