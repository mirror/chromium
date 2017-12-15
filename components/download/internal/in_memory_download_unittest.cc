// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/in_memory_download.h"

#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace download {
namespace {

class MockDelegate : public InMemoryDownload::Delegate {
 public:
  MockDelegate() = default;

  void WaitForCompletion() {
    DCHECK(!run_loop_.running());
    run_loop_.Run();
  }

  // InMemoryDownload::Delegate implementation.
  MOCK_METHOD1(OnDownloadProgress, void(InMemoryDownload*));
  void OnDownloadComplete(InMemoryDownload* download) {
    if (run_loop_.running())
      run_loop_.Quit();
  }

 private:
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(MockDelegate);
};

base::WeakPtr<storage::BlobStorageContext> BlobStorageContextGetter(
    storage::BlobStorageContext* blob_context) {
  DCHECK(blob_context);
  return blob_context->AsWeakPtr();
}

class InMemoryDownloadTest : public testing::Test {
 public:
  InMemoryDownloadTest()
      : network_loop_(base::MessageLoop::TYPE_IO),
        io_loop_(base::MessageLoop::TYPE_IO),
        request_context_getter_(
            new net::TestURLRequestContextGetter(network_loop_.task_runner())) {
  }

  ~InMemoryDownloadTest() override = default;

  void SetUp() override {
    test_server_.ServeFilesFromDirectory(GetTestDataDirectory());
    ASSERT_TRUE(test_server_.Start());
  }

 protected:
  base::FilePath GetTestDataDirectory() {
    base::FilePath test_data_dir;
    EXPECT_TRUE(base::PathService::Get(base::DIR_SOURCE_ROOT, &test_data_dir));
    return test_data_dir.AppendASCII("components/test/data/download");
  }

  // Helper method to create a download with request_params.
  void CreateDownload(const RequestParams& request_params) {
    download_ = std::make_unique<InMemoryDownload>(
        base::GenerateGUID(), request_params, TRAFFIC_ANNOTATION_FOR_TESTS,
        delegate(), request_context_getter_,
        base::BindOnce(&BlobStorageContextGetter, &blob_storage_context_),
        io_loop_.task_runner());
  }

  InMemoryDownload* download() { return download_.get(); }
  MockDelegate* delegate() { return &mock_delegate_; }

  net::EmbeddedTestServer* test_server() { return &test_server_; }

 private:
  // IO loop used by request_context_.
  base::MessageLoop network_loop_;

  // IO loop used by blob.
  base::MessageLoop io_loop_;

  std::unique_ptr<InMemoryDownload> download_;
  MockDelegate mock_delegate_;

  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  storage::BlobStorageContext blob_storage_context_;

  net::EmbeddedTestServer test_server_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryDownloadTest);
};

TEST_F(InMemoryDownloadTest, DownloadTest) {
  RequestParams request_params;
  request_params.url = test_server()->GetURL("/text_data.json");
  CreateDownload(request_params);
  download()->Start();
  delegate()->WaitForCompletion();

  base::FilePath path = GetTestDataDirectory().AppendASCII("text_data.json");
  std::string data;
  EXPECT_TRUE(ReadFileToString(path, &data));
  // EXPECT_EQ(data, *download()->TakeDataAsString());
}

}  // namespace

}  // namespace download
