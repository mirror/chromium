// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_content_file_system_file_stream_reader.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/test/fake_arc_bridge_service.h"
#include "components/arc/test/fake_file_system_instance.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/edk/embedder/embedder.h"
#include "net/base/io_buffer.h"
#include "net/base/test_completion_callback.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

namespace {

constexpr char kArcUrl[] = "content://org.chromium.foo/bar";
constexpr char kData[] = "abcdefghijklmnopqrstuvwxyz";

class ArcFileSystemInstanceTestImpl : public FakeFileSystemInstance {
 public:
  explicit ArcFileSystemInstanceTestImpl(const base::FilePath& file_path)
      : file_path_(file_path) {}

  ~ArcFileSystemInstanceTestImpl() override = default;

  void GetFileSize(const std::string& url,
                   const GetFileSizeCallback& callback) override {
    EXPECT_EQ(kArcUrl, url);
    base::File::Info info;
    EXPECT_TRUE(base::GetFileInfo(file_path_, &info));
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback, info.size));
  }

  void OpenFileToRead(const std::string& url,
                      const OpenFileToReadCallback& callback) override {
    EXPECT_EQ(kArcUrl, url);

    base::File file(file_path_, base::File::FLAG_OPEN | base::File::FLAG_READ);
    EXPECT_TRUE(file.IsValid());

    mojo::edk::ScopedPlatformHandle platform_handle(
        mojo::edk::PlatformHandle(file.TakePlatformFile()));
    MojoHandle wrapped_handle;
    EXPECT_EQ(MOJO_RESULT_OK, mojo::edk::CreatePlatformHandleWrapper(
                                  std::move(platform_handle), &wrapped_handle));

    mojo::ScopedHandle scoped_handle{mojo::Handle(wrapped_handle)};
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(callback, base::Passed(std::move(scoped_handle))));
  }

 private:
  base::FilePath file_path_;

  DISALLOW_COPY_AND_ASSIGN(ArcFileSystemInstanceTestImpl);
};

class ArcContentFileSystemFileStreamReaderTest : public testing::Test {
 public:
  ArcContentFileSystemFileStreamReaderTest() = default;

  ~ArcContentFileSystemFileStreamReaderTest() override = default;

  void SetUp() override {
    mojo::edk::Init();

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    base::FilePath path = temp_dir_.GetPath().AppendASCII("bar");
    ASSERT_TRUE(base::WriteFile(path, kData, arraysize(kData)));

    file_system_ = base::MakeUnique<ArcFileSystemInstanceTestImpl>(path);

    ArcServiceManager::SetArcBridgeServiceForTesting(
        base::MakeUnique<FakeArcBridgeService>());
    arc_service_manager_ = base::MakeUnique<ArcServiceManager>(nullptr);
    arc_service_manager_->arc_bridge_service()->file_system()->SetInstance(
        file_system_.get());
  }

 private:
  base::ScopedTempDir temp_dir_;
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<ArcServiceManager> arc_service_manager_;
  std::unique_ptr<ArcFileSystemInstanceTestImpl> file_system_;

  DISALLOW_COPY_AND_ASSIGN(ArcContentFileSystemFileStreamReaderTest);
};

}  // namespace

TEST_F(ArcContentFileSystemFileStreamReaderTest, Read) {
  ArcContentFileSystemFileStreamReader reader(GURL(kArcUrl), 0);

  auto base_buffer =
      make_scoped_refptr(new net::IOBufferWithSize(arraysize(kData)));
  auto drainable_buffer = make_scoped_refptr(
      new net::DrainableIOBuffer(base_buffer.get(), base_buffer->size()));
  while (drainable_buffer->BytesRemaining()) {
    net::TestCompletionCallback callback;
    int result = callback.GetResult(
        reader.Read(drainable_buffer.get(), drainable_buffer->BytesRemaining(),
                    callback.callback()));
    ASSERT_GT(result, 0);
    drainable_buffer->DidConsume(result);
  }
  EXPECT_EQ(base::StringPiece(kData, arraysize(kData)),
            base::StringPiece(base_buffer->data(), base_buffer->size()));
}

TEST_F(ArcContentFileSystemFileStreamReaderTest, GetLength) {
  ArcContentFileSystemFileStreamReader reader(GURL(kArcUrl), 0);

  net::TestInt64CompletionCallback callback;
  EXPECT_EQ(static_cast<int64_t>(arraysize(kData)),
            callback.GetResult(reader.GetLength(callback.callback())));
}

}  // namespace arc
