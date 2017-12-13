// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_url_store_impl.h"

#include "base/test/scoped_task_environment.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_impl.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace storage {

namespace {

using blink::mojom::BlobPtr;
using blink::mojom::BlobURLStorePtr;

class MockDelegate : public BlobRegistryImpl::Delegate {
 public:
  MockDelegate() = default;
  ~MockDelegate() override = default;

  bool CanReadFile(const base::FilePath& file) override {
    return can_read_file_result;
  }
  bool CanReadFileSystemFile(const FileSystemURL& url) override {
    return can_read_file_system_file_result;
  }
  bool CanCommitURL(const GURL& url) override { return can_commit_url_result; }

  bool can_read_file_result = true;
  bool can_read_file_system_file_result = true;
  bool can_commit_url_result = true;
};

}  // namespace

class BlobURLStoreImplTest : public testing::Test {
 public:
  void SetUp() override {
    context_ = std::make_unique<BlobStorageContext>();

    mojo::edk::SetDefaultProcessErrorCallback(base::BindRepeating(
        &BlobURLStoreImplTest::OnBadMessage, base::Unretained(this)));
  }

  void TearDown() override {
    mojo::edk::SetDefaultProcessErrorCallback(
        mojo::edk::ProcessErrorCallback());
  }

  void OnBadMessage(const std::string& error) {
    bad_messages_.push_back(error);
  }

  BlobPtr CreateBlobFromString(const std::string& uuid,
                               const std::string& contents) {
    BlobDataBuilder builder(uuid);
    builder.set_content_type("text/plain");
    builder.AppendData(contents);
    BlobPtr blob;
    BlobImpl::Create(context_->AddFinishedBlob(builder), MakeRequest(&blob));
    return blob;
  }

  std::string UUIDFromBlob(blink::mojom::Blob* blob) {
    base::RunLoop loop;
    std::string received_uuid;
    blob->GetInternalUUID(base::BindOnce(
        [](base::OnceClosure quit_closure, std::string* uuid_out,
           const std::string& uuid) {
          *uuid_out = uuid;
          std::move(quit_closure).Run();
        },
        loop.QuitClosure(), &received_uuid));
    loop.Run();
    return received_uuid;
  }

  BlobURLStorePtr CreateURLStore() {
    BlobURLStorePtr result;
    mojo::MakeStrongBinding(
        std::make_unique<BlobURLStoreImpl>(context_->AsWeakPtr(), &delegate_),
        MakeRequest(&result));
    return result;
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<BlobStorageContext> context_;
  MockDelegate delegate_;
  std::vector<std::string> bad_messages_;
};

TEST_F(BlobURLStoreImplTest, BasicRegisterRevoke) {
  const std::string kId = "id";
  const GURL kUrl("blob:id");
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  // Register a URL and make sure the URL keeps the blob alive.
  BlobURLStoreImpl url_store(context_->AsWeakPtr(), &delegate_);
  base::RunLoop loop;
  url_store.Register(std::move(blob), kUrl, loop.QuitClosure());
  loop.Run();

  std::unique_ptr<BlobDataHandle> blob_data_handle =
      context_->GetBlobDataFromPublicURL(kUrl);
  ASSERT_TRUE(blob_data_handle);
  EXPECT_EQ(kId, blob_data_handle->uuid());
  blob_data_handle = nullptr;
  base::RunLoop().RunUntilIdle();

  // Revoke the URL.
  url_store.Revoke(kUrl);
  blob_data_handle = context_->GetBlobDataFromPublicURL(kUrl);
  EXPECT_FALSE(blob_data_handle);
  EXPECT_FALSE(context_->registry().HasEntry(kId));
}

TEST_F(BlobURLStoreImplTest, RegisterInvalidScheme) {
  const std::string kId = "id";
  const GURL kInvalidUrl("bolb:id");
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  BlobURLStorePtr url_store = CreateURLStore();
  {
    base::RunLoop loop;
    url_store->Register(std::move(blob), kInvalidUrl, loop.QuitClosure());
    loop.Run();
  }
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kInvalidUrl));
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(BlobURLStoreImplTest, RegisterCantCommit) {
  const std::string kId = "id";
  const GURL kUrl("blob:id");
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  delegate_.can_commit_url_result = false;

  BlobURLStorePtr url_store = CreateURLStore();
  {
    base::RunLoop loop;
    url_store->Register(std::move(blob), kUrl, loop.QuitClosure());
    loop.Run();
  }
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kUrl));
  EXPECT_EQ(1u, bad_messages_.size());
}

TEST_F(BlobURLStoreImplTest, ImplicitRevoke) {
  const std::string kId = "id";
  const GURL kUrl1("blob:id1");
  const GURL kUrl2("blob:id2");
  BlobPtr blob = CreateBlobFromString(kId, "hello world");
  BlobPtr blob2;
  blob->Clone(MakeRequest(&blob2));

  auto url_store =
      std::make_unique<BlobURLStoreImpl>(context_->AsWeakPtr(), &delegate_);
  {
    base::RunLoop loop;
    url_store->Register(std::move(blob), kUrl1, loop.QuitClosure());
    loop.Run();
    EXPECT_TRUE(context_->GetBlobDataFromPublicURL(kUrl1));
  }
  {
    base::RunLoop loop;
    url_store->Register(std::move(blob2), kUrl2, loop.QuitClosure());
    loop.Run();
    EXPECT_TRUE(context_->GetBlobDataFromPublicURL(kUrl2));
  }

  // Destroy URL Store, should revoke URLs.
  url_store = nullptr;
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kUrl1));
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kUrl2));
}

TEST_F(BlobURLStoreImplTest, RevokeThroughDifferentURLStore) {
  const std::string kId = "id";
  const GURL kUrl("blob:id");
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  BlobURLStoreImpl url_store1(context_->AsWeakPtr(), &delegate_);
  BlobURLStoreImpl url_store2(context_->AsWeakPtr(), &delegate_);
  base::RunLoop loop;
  url_store1.Register(std::move(blob), kUrl, loop.QuitClosure());
  loop.Run();
  EXPECT_TRUE(context_->GetBlobDataFromPublicURL(kUrl));

  url_store2.Revoke(kUrl);
  EXPECT_FALSE(context_->GetBlobDataFromPublicURL(kUrl));
}

TEST_F(BlobURLStoreImplTest, Resolve) {
  const std::string kId = "id";
  const GURL kUrl("blob:id");
  BlobPtr blob = CreateBlobFromString(kId, "hello world");

  BlobURLStoreImpl url_store(context_->AsWeakPtr(), &delegate_);
  {
    base::RunLoop loop;
    url_store.Register(std::move(blob), kUrl, loop.QuitClosure());
    loop.Run();
  }

  {
    base::RunLoop loop;
    url_store.Resolve(
        kUrl, base::BindOnce(
                  [](base::OnceClosure done, BlobPtr* blob_out, BlobPtr blob) {
                    *blob_out = std::move(blob);
                    std::move(done).Run();
                  },
                  loop.QuitClosure(), &blob));
    loop.Run();
  }

  ASSERT_TRUE(blob);
  EXPECT_EQ(kId, UUIDFromBlob(blob.get()));
}

TEST_F(BlobURLStoreImplTest, ResolveInvalidURL) {
  BlobURLStoreImpl url_store(context_->AsWeakPtr(), &delegate_);

  BlobPtr blob;
  {
    base::RunLoop loop;
    url_store.Resolve(GURL("blob:id"), base::BindOnce(
                                           [](base::OnceClosure done,
                                              BlobPtr* blob_out, BlobPtr blob) {
                                             *blob_out = std::move(blob);
                                             std::move(done).Run();
                                           },
                                           loop.QuitClosure(), &blob));
    loop.Run();
  }
  EXPECT_FALSE(blob);
}

}  // namespace storage
