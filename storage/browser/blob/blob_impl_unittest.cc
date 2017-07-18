// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_impl.h"

#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/common/data_pipe_drainer.h"
#include "net/base/net_errors.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace storage {

namespace {

class DataPipeReader : public mojo::common::DataPipeDrainer::Client {
 public:
  DataPipeReader(std::string* data_out, base::OnceClosure done_callback)
      : data_out_(data_out), done_callback_(std::move(done_callback)) {}

  void OnDataAvailable(const void* data, size_t num_bytes) override {
    data_out_->append(static_cast<const char*>(data), num_bytes);
  }

  void OnDataComplete() override { std::move(done_callback_).Run(); }

 private:
  std::string* data_out_;
  base::OnceClosure done_callback_;
};

}  // namespace

class BlobImplTest : public testing::Test {
 public:
  void SetUp() override { context_ = base::MakeUnique<BlobStorageContext>(); }

  std::unique_ptr<BlobDataHandle> CreateBlobFromString(
      const std::string& uuid,
      const std::string& contents) {
    BlobDataBuilder builder(uuid);
    builder.set_content_type("text/plain");
    builder.AppendData(contents);
    return context_->AddFinishedBlob(builder);
  }

  std::string UUIDFromBlob(mojom::Blob* blob) {
    base::RunLoop loop;
    std::string received_uuid;
    blob->GetInternalUUID(base::Bind(
        [](base::Closure quit_closure, std::string* uuid_out,
           const std::string& uuid) {
          *uuid_out = uuid;
          quit_closure.Run();
        },
        loop.QuitClosure(), &received_uuid));
    loop.Run();
    return received_uuid;
  }

  std::string ReadDataPipe(mojo::ScopedDataPipeConsumerHandle pipe) {
    base::RunLoop loop;
    std::string data;
    DataPipeReader reader(&data, loop.QuitClosure());
    mojo::common::DataPipeDrainer drainer(&reader, std::move(pipe));
    loop.Run();
    return data;
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<BlobStorageContext> context_;
};

TEST_F(BlobImplTest, GetInternalUUID) {
  const std::string kId = "id";
  auto handle = CreateBlobFromString(kId, "hello world");

  mojom::BlobPtr ptr;
  auto blob = BlobImpl::Create(std::move(handle), MakeRequest(&ptr));
  EXPECT_EQ(kId, UUIDFromBlob(blob.get()));
  EXPECT_EQ(kId, UUIDFromBlob(ptr.get()));
}

TEST_F(BlobImplTest, CloneAndLifetime) {
  const std::string kId = "id";
  auto handle = CreateBlobFromString(kId, "hello world");

  mojom::BlobPtr ptr;
  auto blob = BlobImpl::Create(std::move(handle), MakeRequest(&ptr));
  EXPECT_EQ(kId, UUIDFromBlob(ptr.get()));

  // Blob should exist in registry as long as connection is alive.
  EXPECT_TRUE(context_->registry().HasEntry(kId));
  EXPECT_TRUE(blob);

  mojom::BlobPtr clone;
  blob->Clone(MakeRequest(&clone));
  EXPECT_EQ(kId, UUIDFromBlob(clone.get()));
  clone.FlushForTesting();

  ptr.reset();
  blob->FlushForTesting();
  EXPECT_TRUE(context_->registry().HasEntry(kId));
  EXPECT_TRUE(blob);

  clone.reset();
  blob->FlushForTesting();
  EXPECT_FALSE(context_->registry().HasEntry(kId));
  EXPECT_FALSE(blob);
}

TEST_F(BlobImplTest, ReadAll) {
  const std::string kId = "id";
  const std::string kContents = "hello world";
  auto handle = CreateBlobFromString(kId, kContents);

  mojom::BlobPtr ptr;
  BlobImpl::Create(std::move(handle), MakeRequest(&ptr));

  base::RunLoop loop;
  mojo::DataPipe pipe;
  mojom::BlobReadResultPtr read_result;
  ptr->ReadAll(std::move(pipe.producer_handle),
               base::BindOnce(
                   [](base::OnceClosure quit_closure,
                      mojom::BlobReadResultPtr* result_out,
                      mojom::BlobReadResultPtr result) {
                     *result_out = std::move(result);
                     std::move(quit_closure).Run();
                   },
                   loop.QuitClosure(), &read_result));

  std::string received = ReadDataPipe(std::move(pipe.consumer_handle));
  EXPECT_EQ(kContents, received);

  loop.Run();
  EXPECT_EQ(true, read_result->success);
  EXPECT_EQ(net::OK, read_result->error);
  EXPECT_EQ(kContents.size(), read_result->bytes_read);
}

TEST_F(BlobImplTest, ReadAll_BrokenBlob) {
  const std::string kId = "id";
  auto handle = context_->AddBrokenBlob(
      kId, "", "", BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS);

  mojom::BlobPtr ptr;
  BlobImpl::Create(std::move(handle), MakeRequest(&ptr));

  base::RunLoop loop;
  mojo::DataPipe pipe;
  mojom::BlobReadResultPtr read_result;
  ptr->ReadAll(std::move(pipe.producer_handle),
               base::BindOnce(
                   [](base::OnceClosure quit_closure,
                      mojom::BlobReadResultPtr* result_out,
                      mojom::BlobReadResultPtr result) {
                     *result_out = std::move(result);
                     std::move(quit_closure).Run();
                   },
                   loop.QuitClosure(), &read_result));

  std::string received = ReadDataPipe(std::move(pipe.consumer_handle));
  EXPECT_EQ("", received);

  loop.Run();
  EXPECT_EQ(false, read_result->success);
  EXPECT_EQ(net::ERR_FAILED, read_result->error);
  EXPECT_EQ(0u, read_result->bytes_read);
}

TEST_F(BlobImplTest, ReadRange) {
  const std::string kId = "id";
  const std::string kContents = "hello world";
  auto handle = CreateBlobFromString(kId, kContents);

  mojom::BlobPtr ptr;
  BlobImpl::Create(std::move(handle), MakeRequest(&ptr));

  base::RunLoop loop;
  mojo::DataPipe pipe;
  mojom::BlobReadResultPtr read_result;
  ptr->ReadRange(2, 5, std::move(pipe.producer_handle),
                 base::BindOnce(
                     [](base::OnceClosure quit_closure,
                        mojom::BlobReadResultPtr* result_out,
                        mojom::BlobReadResultPtr result) {
                       *result_out = std::move(result);
                       std::move(quit_closure).Run();
                     },
                     loop.QuitClosure(), &read_result));

  std::string received = ReadDataPipe(std::move(pipe.consumer_handle));
  EXPECT_EQ(kContents.substr(2, 5), received);

  loop.Run();
  EXPECT_EQ(true, read_result->success);
  EXPECT_EQ(net::OK, read_result->error);
  EXPECT_EQ(5u, read_result->bytes_read);
}

TEST_F(BlobImplTest, ReadRange_BrokenBlob) {
  const std::string kId = "id";
  auto handle = context_->AddBrokenBlob(
      kId, "", "", BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS);

  mojom::BlobPtr ptr;
  BlobImpl::Create(std::move(handle), MakeRequest(&ptr));

  base::RunLoop loop;
  mojo::DataPipe pipe;
  mojom::BlobReadResultPtr read_result;
  ptr->ReadRange(2, 5, std::move(pipe.producer_handle),
                 base::BindOnce(
                     [](base::OnceClosure quit_closure,
                        mojom::BlobReadResultPtr* result_out,
                        mojom::BlobReadResultPtr result) {
                       *result_out = std::move(result);
                       std::move(quit_closure).Run();
                     },
                     loop.QuitClosure(), &read_result));

  std::string received = ReadDataPipe(std::move(pipe.consumer_handle));
  EXPECT_EQ("", received);

  loop.Run();
  EXPECT_EQ(false, read_result->success);
  EXPECT_EQ(net::ERR_FAILED, read_result->error);
  EXPECT_EQ(0u, read_result->bytes_read);
}

TEST_F(BlobImplTest, ReadRange_InvalidRange) {
  const std::string kId = "id";
  const std::string kContents = "hello world";
  auto handle = CreateBlobFromString(kId, kContents);

  mojom::BlobPtr ptr;
  BlobImpl::Create(std::move(handle), MakeRequest(&ptr));

  base::RunLoop loop;
  mojo::DataPipe pipe;
  mojom::BlobReadResultPtr read_result;
  ptr->ReadRange(15, 4, std::move(pipe.producer_handle),
                 base::BindOnce(
                     [](base::OnceClosure quit_closure,
                        mojom::BlobReadResultPtr* result_out,
                        mojom::BlobReadResultPtr result) {
                       *result_out = std::move(result);
                       std::move(quit_closure).Run();
                     },
                     loop.QuitClosure(), &read_result));

  std::string received = ReadDataPipe(std::move(pipe.consumer_handle));
  EXPECT_EQ("", received);

  loop.Run();
  EXPECT_EQ(false, read_result->success);
  EXPECT_EQ(net::ERR_REQUEST_RANGE_NOT_SATISFIABLE, read_result->error);
  EXPECT_EQ(0u, read_result->bytes_read);
}

}  // namespace storage
