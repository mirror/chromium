// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_use_measurement/content/service_url_loader.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/test/histogram_tester.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

namespace data_use_measurement {

class MockSimpleURLLoader : public content::SimpleURLLoader {
 public:
  MockSimpleURLLoader() = default;
  ~MockSimpleURLLoader() = default;

  // GMock doesn't support move only params, such as OnceCallbacks. Use a
  // pointer to mock those methods.
  void DownloadToString(
      network::mojom::URLLoaderFactory* url_loader_factory,
      content::SimpleURLLoader::BodyAsStringCallback body_as_string_callback,
      size_t max_body_size) override {
    DownloadToString(url_loader_factory, &body_as_string_callback,
                     max_body_size);
  }
  void DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      network::mojom::URLLoaderFactory* url_loader_factory,
      content::SimpleURLLoader::BodyAsStringCallback body_as_string_callback)
      override {
    DownloadToStringOfUnboundedSizeUntilCrashAndDie(url_loader_factory,
                                                    &body_as_string_callback);
  }

  void DownloadToFile(network::mojom::URLLoaderFactory* url_loader_factory,
                      content::SimpleURLLoader::DownloadToFileCompleteCallback
                          download_to_file_complete_callback,
                      const base::FilePath& file_path,
                      int64_t max_body_size) override {
    DownloadToFile(url_loader_factory, &download_to_file_complete_callback,
                   file_path, max_body_size);
  }

  void DownloadToTempFile(
      network::mojom::URLLoaderFactory* url_loader_factory,
      content::SimpleURLLoader::DownloadToFileCompleteCallback
          download_to_file_complete_callback,
      int64_t max_body_size) override {
    DownloadToTempFile(url_loader_factory, &download_to_file_complete_callback,
                       max_body_size);
  }

  MOCK_METHOD3(DownloadToString,
               void(network::mojom::URLLoaderFactory*,
                    content::SimpleURLLoader::BodyAsStringCallback*,
                    size_t));
  MOCK_METHOD2(DownloadToStringOfUnboundedSizeUntilCrashAndDie,
               void(network::mojom::URLLoaderFactory*,
                    content::SimpleURLLoader::BodyAsStringCallback*));
  MOCK_METHOD4(DownloadToFile,
               void(network::mojom::URLLoaderFactory*,
                    content::SimpleURLLoader::DownloadToFileCompleteCallback*,
                    const base::FilePath&,
                    int64_t));
  MOCK_METHOD3(DownloadToTempFile,
               void(network::mojom::URLLoaderFactory*,
                    content::SimpleURLLoader::DownloadToFileCompleteCallback*,
                    int64_t));
  MOCK_METHOD1(SetOnRedirectCallback,
               void(const content::SimpleURLLoader::OnRedirectCallback&));
  MOCK_METHOD1(SetAllowPartialResults, void(bool));
  MOCK_METHOD1(SetAllowHttpErrorResults, void(bool));
  MOCK_METHOD2(AttachStringForUpload,
               void(const std::string&, const std::string&));
  MOCK_METHOD2(AttachFileForUpload,
               void(const base::FilePath&, const std::string&));
  MOCK_METHOD2(SetRetryOptions, void(int, int));
  MOCK_CONST_METHOD0(NetError, int());
  MOCK_CONST_METHOD0(ResponseInfo, const network::ResourceResponseHead*());
};

class ServiceURLLoaderTest : public testing::Test {
 public:
  ServiceURLLoaderTest() {}
  ~ServiceURLLoaderTest() override {}

  void SetUp() override {
    std::unique_ptr<MockSimpleURLLoader> mock_loader =
        std::make_unique<MockSimpleURLLoader>();
    mock_simple_url_loader_ = mock_loader.get();
    service_url_loader_ = std::make_unique<ServiceURLLoader>(
        std::move(mock_loader),
        ServiceURLLoader::ServiceName::kDataReductionProxy);
  }

  ServiceURLLoader* service_url_loader() { return service_url_loader_.get(); }

  MockSimpleURLLoader& mock_simple_url_loader() {
    return *mock_simple_url_loader_;
  }

  base::HistogramTester& histogram_tester() { return histogram_tester_; }

 private:
  base::HistogramTester histogram_tester_;
  MockSimpleURLLoader* mock_simple_url_loader_;
  std::unique_ptr<ServiceURLLoader> service_url_loader_;
};

TEST_F(ServiceURLLoaderTest, CheckHistogram) {
  histogram_tester().ExpectUniqueSample(
      "ServiceURLLoader.ServiceType",
      ServiceURLLoader::ServiceName::kDataReductionProxy, 1);
}

TEST_F(ServiceURLLoaderTest, DownloadToStringPassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), DownloadToString(_, _, _));
  service_url_loader()->DownloadToString(
      nullptr, content::SimpleURLLoader::BodyAsStringCallback(), 0);
}

TEST_F(ServiceURLLoaderTest,
       DownloadToStringOfUnboundedSizeUntilCrashAndDiePassThrough) {
  EXPECT_CALL(mock_simple_url_loader(),
              DownloadToStringOfUnboundedSizeUntilCrashAndDie(_, _));
  service_url_loader()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      nullptr, content::SimpleURLLoader::BodyAsStringCallback());
}

TEST_F(ServiceURLLoaderTest, DownloadToFilePassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), DownloadToFile(_, _, _, _));
  service_url_loader()->DownloadToFile(
      nullptr, content::SimpleURLLoader::DownloadToFileCompleteCallback(),
      base::FilePath(), 0);
}

TEST_F(ServiceURLLoaderTest, DownloadToTempFilePassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), DownloadToTempFile(_, _, _));
  service_url_loader()->DownloadToTempFile(
      nullptr, content::SimpleURLLoader::DownloadToFileCompleteCallback(), 0);
}

TEST_F(ServiceURLLoaderTest, SetOnRedirectCallbackPassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), SetOnRedirectCallback(_));
  service_url_loader()->SetOnRedirectCallback(
      content::SimpleURLLoader::OnRedirectCallback());
}

TEST_F(ServiceURLLoaderTest, SetAllowPartialResultsPassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), SetAllowPartialResults(_));
  service_url_loader()->SetAllowPartialResults(true);
}

TEST_F(ServiceURLLoaderTest, SetAllowHttpErrorResultsPassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), SetAllowHttpErrorResults(_));
  service_url_loader()->SetAllowHttpErrorResults(true);
}

TEST_F(ServiceURLLoaderTest, AttachStringForUploadPassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), AttachStringForUpload(_, _));
  service_url_loader()->AttachStringForUpload(std::string(), std::string());
}

TEST_F(ServiceURLLoaderTest, AttachFileForUploadPassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), AttachFileForUpload(_, _));
  service_url_loader()->AttachFileForUpload(base::FilePath(), std::string());
}

TEST_F(ServiceURLLoaderTest, SetRetryOptionsPassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), SetRetryOptions(_, _));
  service_url_loader()->SetRetryOptions(0, 0);
}

TEST_F(ServiceURLLoaderTest, NetErrorPassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), NetError());
  service_url_loader()->NetError();
}

TEST_F(ServiceURLLoaderTest, ResponseInfoPassThrough) {
  EXPECT_CALL(mock_simple_url_loader(), ResponseInfo());
  service_url_loader()->ResponseInfo();
}

}  // namespace data_use_measurement
