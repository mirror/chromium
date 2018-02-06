// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_cert_fetcher.h"

#include "base/optional.h"
#include "base/strings/string_piece.h"
#include "base/test/scoped_task_environment.h"
#include "content/common/weak_wrapper_shared_url_loader_factory.h"
#include "content/public/common/resource_type.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/load_flags.h"
#include "net/cert/x509_util.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "services/network/public/interfaces/url_loader_factory.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

base::Optional<std::vector<base::StringPiece>> GetCertChain(
    const uint8_t* input,
    size_t input_size) {
  return SignedExchangeCertFetcher::GetCertChainFromMessage(
      base::StringPiece(reinterpret_cast<const char*>(input), input_size));
}

class MockURLLoader final : public network::mojom::URLLoader {
 public:
  MockURLLoader(network::mojom::URLLoaderRequest url_loader_request)
      : binding_(this, std::move(url_loader_request)) {}
  ~MockURLLoader() override = default;

  MOCK_METHOD0(FollowRedirect, void());
  MOCK_METHOD0(ProceedWithResponse, void());
  MOCK_METHOD2(SetPriority,
               void(net::RequestPriority priority,
                    int32_t intra_priority_value));
  MOCK_METHOD0(PauseReadingBodyFromNet, void());
  MOCK_METHOD0(ResumeReadingBodyFromNet, void());

 private:
  mojo::Binding<network::mojom::URLLoader> binding_;

  DISALLOW_COPY_AND_ASSIGN(MockURLLoader);
};

class URLLoaderFactoryForMockLoader final
    : public network::mojom::URLLoaderFactory {
 public:
  URLLoaderFactoryForMockLoader() = default;
  ~URLLoaderFactoryForMockLoader() override = default;

  // network::mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest url_loader_request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& url_request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    loader_ = std::make_unique<MockURLLoader>(std::move(url_loader_request));
    url_request_ = url_request;
    client_ptr_ = std::move(client);
  }

  void Clone(network::mojom::URLLoaderFactoryRequest factory) override {
    NOTREACHED();
  }

  network::mojom::URLLoaderClientPtr& client_ptr() { return client_ptr_; }
  void CloseClientPipe() { client_ptr_.reset(); }

  base::Optional<network::ResourceRequest> url_request() const {
    return url_request_;
  }

 private:
  std::unique_ptr<MockURLLoader> loader_;
  network::mojom::URLLoaderClientPtr client_ptr_;
  base::Optional<network::ResourceRequest> url_request_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderFactoryForMockLoader);
};

void ForwardCertificateCallback(bool* called,
                                scoped_refptr<net::X509Certificate>* out_cert,
                                scoped_refptr<net::X509Certificate> cert) {
  *called = true;
  *out_cert = std::move(cert);
}

class SignedExchangeCertFetcherTest : public testing::Test {
 public:
  SignedExchangeCertFetcherTest()
      : url_(GURL("https://www.example.com/cert")) {}
  ~SignedExchangeCertFetcherTest() override {}

 protected:
  static scoped_refptr<net::X509Certificate> ImportTestCert() {
    return net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
  }

  static std::string CreateCertMessage(const base::StringPiece& cert_data) {
    std::string message;
    uint32_t cert_size = cert_data.length();
    uint32_t cert_list_size = cert_size + 3 /* size of "cert data size" */ +
                              2 /* size of "extensions size" */;
    uint32_t message_size = cert_list_size +
                            1 /* size of "request context size" */ +
                            3 /* size of "certificate list size" */;
    // request context size
    message += static_cast<char>(0x00);
    // certificate list size
    message += static_cast<char>(cert_list_size >> 16);
    message += static_cast<char>((cert_list_size & 0xFF00) >> 8);
    message += static_cast<char>(cert_list_size & 0xFF);
    // certificate list size
    message += static_cast<char>(cert_size >> 16);
    message += static_cast<char>((cert_size & 0xFF00) >> 8);
    message += static_cast<char>(cert_size & 0xFF);
    // cert data
    message += std::string(cert_data);
    // extensions size
    message += static_cast<char>(0x00);
    message += static_cast<char>(0x00);
    CHECK_EQ(message_size, message.size());
    return message;
  }

  static base::StringPiece CreateCertMessageFromCert(
      const net::X509Certificate& cert) {
    return net::x509_util::CryptoBufferAsStringPiece(cert.cert_buffer());
  }

  static mojo::ScopedDataPipeConsumerHandle CreateTestDataFilledDataPipe() {
    scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
    const std::string message =
        CreateCertMessage(CreateCertMessageFromCert(*certificate));

    mojo::DataPipe data_pipe(message.size());
    CHECK(mojo::common::BlockingCopyFromString(message,
                                               data_pipe.producer_handle));
    return std::move(data_pipe.consumer_handle);
  }

  static net::SHA256HashValue GetTestDataCertFingerprint256() {
    return ImportTestCert()->CalculateChainFingerprint256();
  }

  void RunUntilIdle() { scoped_task_environment_.RunUntilIdle(); }

  std::unique_ptr<SignedExchangeCertFetcher> CreateFetcherAndStart(
      bool force_fetch) {
    SignedExchangeCertFetcher::CertificateCallback callback = base::BindOnce(
        &ForwardCertificateCallback, base::Unretained(&callback_called_),
        base::Unretained(&cert_result_));

    return SignedExchangeCertFetcher::CreateAndStart(
        base::MakeRefCounted<WeakWrapperSharedURLLoaderFactory>(
            &mock_loader_factory_),
        url_, force_fetch, std::move(callback));
  }

  void CallOnReceiveResponse() {
    network::ResourceResponseHead resource_response;
    resource_response.headers =
        base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");

    mock_loader_factory_.client_ptr()->OnReceiveResponse(
        resource_response, base::Optional<net::SSLInfo>(),
        nullptr /* downloaded_file */);
  }

  void CloseClientPipe() { mock_loader_factory_.CloseClientPipe(); }

  const GURL url_;
  bool callback_called_ = false;
  scoped_refptr<net::X509Certificate> cert_result_;
  URLLoaderFactoryForMockLoader mock_loader_factory_;

  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SignedExchangeCertFetcherTest);
};

}  // namespace

TEST(SignedExchangeCertFetcherParseTest, OneCert) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x07, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, 0x00, // extensions size
      // clang-format on
  };
  base::Optional<std::vector<base::StringPiece>> certs =
      GetCertChain(input, arraysize(input));
  ASSERT_TRUE(certs);
  ASSERT_EQ(1u, certs->size());
  const uint8_t kExpected[] = {
      // clang-format off
      0x11, 0x22, // cert data
      // clang-format on
  };
  EXPECT_THAT((*certs)[0],
              testing::ElementsAreArray(kExpected, arraysize(kExpected)));
}

TEST(SignedExchangeCertFetcherParseTest, OneCertWithExtension) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x0A, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, 0x03, // extensions size
      0xE1, 0xE2, 0xE3, // extensions data
      // clang-format on
  };
  base::Optional<std::vector<base::StringPiece>> certs =
      GetCertChain(input, arraysize(input));
  ASSERT_TRUE(certs);
  ASSERT_EQ(1u, certs->size());
  const uint8_t kExpected[] = {
      // clang-format off
      0x11, 0x22, // cert data
      // clang-format on
  };
  EXPECT_THAT((*certs)[0],
              testing::ElementsAreArray(kExpected, arraysize(kExpected)));
}

TEST(SignedExchangeCertFetcherParseTest, TwoCerts) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x01, 0x13, // certificate list size

      0x00, 0x01, 0x04, // cert data size

      // cert data
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,

      0x00, 0x00, // extensions size

      0x00, 0x00, 0x05, // cert data size
      0x33, 0x44, 0x55, 0x66, 0x77, // cert data
      0x00, 0x00, // extensions size

      // clang-format on
  };

  const uint8_t kExpected1[] = {
      // clang-format off
      // cert data
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
      // clang-format on
  };
  const uint8_t kExpected2[] = {
      // clang-format off
      0x33, 0x44, 0x55, 0x66, 0x77, // cert data
      // clang-format on
  };

  base::Optional<std::vector<base::StringPiece>> certs =
      GetCertChain(input, sizeof(input));
  ASSERT_TRUE(certs);
  ASSERT_EQ(2u, certs->size());
  EXPECT_THAT((*certs)[0],
              testing::ElementsAreArray(kExpected1, arraysize(kExpected1)));
  EXPECT_THAT((*certs)[1],
              testing::ElementsAreArray(kExpected2, arraysize(kExpected2)));
}

TEST(SignedExchangeCertFetcherParseTest, Empty) {
  EXPECT_FALSE(GetCertChain(nullptr, 0));
}

TEST(SignedExchangeCertFetcherParseTest, InvalidRequestContextSize) {
  const uint8_t input[] = {
      // clang-format off
      0x01, // request context size: must be zero
      0x20, // request context
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertFetcherParseTest, CanNotReadCertListSize1) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x01, // certificate list size: must be 3 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertFetcherParseTest, CanNotReadCertListSize2) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x01, // certificate list size: must be 3 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertFetcherParseTest, CertListSizeError) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x01, 0x01, // certificate list size: 257 (This must be 7)

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, 0x00, // extensions size
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertFetcherParseTest, CanNotReadCertDataSize) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x02, // certificate list size

      0x00, 0x01, // cert data size: must be 3 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertFetcherParseTest, CertDataSizeError) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x04, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, // cert data: Need 2 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertFetcherParseTest, CanNotReadExtensionsSize) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x06, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, // extensions size : must be 2 bytes
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST(SignedExchangeCertFetcherParseTest, ExtensionsSizeError) {
  const uint8_t input[] = {
      // clang-format off
      0x00, // request context size
      0x00, 0x00, 0x07, // certificate list size

      0x00, 0x00, 0x02, // cert data size
      0x11, 0x22, // cert data
      0x00, 0x01, // extensions size
      // clang-format on
  };
  EXPECT_FALSE(GetCertChain(input, arraysize(input)));
}

TEST_F(SignedExchangeCertFetcherTest, Simple) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);

  ASSERT_TRUE(mock_loader_factory_.client_ptr());
  ASSERT_TRUE(mock_loader_factory_.url_request());
  EXPECT_EQ(url_, mock_loader_factory_.url_request()->url);
  EXPECT_EQ(net::LOAD_DO_NOT_SEND_AUTH_DATA | net::LOAD_DO_NOT_SAVE_COOKIES |
                net::LOAD_DO_NOT_SEND_COOKIES,
            mock_loader_factory_.url_request()->load_flags);

  CallOnReceiveResponse();
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      CreateTestDataFilledDataPipe());
  mock_loader_factory_.client_ptr()->OnComplete(
      network::URLLoaderCompletionStatus(net::OK));
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  ASSERT_TRUE(cert_result_);
  EXPECT_EQ(GetTestDataCertFingerprint256(),
            cert_result_->CalculateChainFingerprint256());
}

TEST_F(SignedExchangeCertFetcherTest, MultipleChunked) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
  const std::string message =
      CreateCertMessage(CreateCertMessageFromCert(*certificate));
  mojo::DataPipe data_pipe(message.size() / 2 + 1);
  CHECK(mojo::common::BlockingCopyFromString(
      message.substr(0, message.size() / 2), data_pipe.producer_handle));
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  RunUntilIdle();
  CHECK(mojo::common::BlockingCopyFromString(message.substr(message.size() / 2),
                                             data_pipe.producer_handle));
  data_pipe.producer_handle.reset();
  mock_loader_factory_.client_ptr()->OnComplete(
      network::URLLoaderCompletionStatus(net::OK));
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  ASSERT_TRUE(cert_result_);
  EXPECT_EQ(certificate->CalculateChainFingerprint256(),
            cert_result_->CalculateChainFingerprint256());
}

TEST_F(SignedExchangeCertFetcherTest, ForceFetchAndFail) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(true /* force_fetch */);
  CallOnReceiveResponse();

  ASSERT_TRUE(mock_loader_factory_.url_request());
  EXPECT_EQ(url_, mock_loader_factory_.url_request()->url);
  EXPECT_EQ(net::LOAD_DO_NOT_SEND_AUTH_DATA | net::LOAD_DO_NOT_SAVE_COOKIES |
                net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DISABLE_CACHE |
                net::LOAD_BYPASS_CACHE,
            mock_loader_factory_.url_request()->load_flags);

  mock_loader_factory_.client_ptr()->OnComplete(
      network::URLLoaderCompletionStatus(net::ERR_FAILED));
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, MaxCertSize_Exceeds) {
  scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
  const std::string message =
      CreateCertMessage(CreateCertMessageFromCert(*certificate));
  base::ScopedClosureRunner reset_max =
      SignedExchangeCertFetcher::SetMaxCertSizeForTest(message.size() - 1);

  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  mojo::DataPipe data_pipe(message.size());
  CHECK(
      mojo::common::BlockingCopyFromString(message, data_pipe.producer_handle));
  data_pipe.producer_handle.reset();
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  mock_loader_factory_.client_ptr()->OnComplete(
      network::URLLoaderCompletionStatus(net::OK));
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, MaxCertSize_SameSize) {
  scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
  const std::string message =
      CreateCertMessage(CreateCertMessageFromCert(*certificate));
  base::ScopedClosureRunner reset_max =
      SignedExchangeCertFetcher::SetMaxCertSizeForTest(message.size());

  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  mojo::DataPipe data_pipe(message.size());
  CHECK(
      mojo::common::BlockingCopyFromString(message, data_pipe.producer_handle));
  data_pipe.producer_handle.reset();
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  mock_loader_factory_.client_ptr()->OnComplete(
      network::URLLoaderCompletionStatus(net::OK));
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_TRUE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, MaxCertSize_MultipleChunked) {
  scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
  const std::string message =
      CreateCertMessage(CreateCertMessageFromCert(*certificate));
  base::ScopedClosureRunner reset_max =
      SignedExchangeCertFetcher::SetMaxCertSizeForTest(message.size() - 1);

  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  mojo::DataPipe data_pipe(message.size() / 2 + 1);
  CHECK(mojo::common::BlockingCopyFromString(
      message.substr(0, message.size() / 2), data_pipe.producer_handle));
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  RunUntilIdle();
  CHECK(mojo::common::BlockingCopyFromString(message.substr(message.size() / 2),
                                             data_pipe.producer_handle));
  data_pipe.producer_handle.reset();
  mock_loader_factory_.client_ptr()->OnComplete(
      network::URLLoaderCompletionStatus(net::OK));
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, MaxCertSize_ContentLengthCheck) {
  scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
  const std::string message =
      CreateCertMessage(CreateCertMessageFromCert(*certificate));
  base::ScopedClosureRunner reset_max =
      SignedExchangeCertFetcher::SetMaxCertSizeForTest(message.size() - 1);

  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  network::ResourceResponseHead resource_response;
  resource_response.headers =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  resource_response.content_length = message.size();
  mock_loader_factory_.client_ptr()->OnReceiveResponse(
      resource_response, base::Optional<net::SSLInfo>(),
      nullptr /* downloaded_file */);
  mojo::DataPipe data_pipe(message.size());
  CHECK(
      mojo::common::BlockingCopyFromString(message, data_pipe.producer_handle));
  data_pipe.producer_handle.reset();
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  mock_loader_factory_.client_ptr()->OnComplete(
      network::URLLoaderCompletionStatus(net::OK));
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, Abort_Redirect) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  network::ResourceResponseHead response_head;
  net::RedirectInfo redirect_info;
  mock_loader_factory_.client_ptr()->OnReceiveRedirect(redirect_info,
                                                       response_head);
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, Abort_404) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  network::ResourceResponseHead resource_response;
  resource_response.headers =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 404 Not Found");
  mock_loader_factory_.client_ptr()->OnReceiveResponse(
      resource_response, base::Optional<net::SSLInfo>(),
      nullptr /* downloaded_file */);
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, Invalid_CertData) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  const std::string message = CreateCertMessage("Invalid Cert Data");
  mojo::DataPipe data_pipe(message.size());
  CHECK(
      mojo::common::BlockingCopyFromString(message, data_pipe.producer_handle));
  data_pipe.producer_handle.reset();
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  mock_loader_factory_.client_ptr()->OnComplete(
      network::URLLoaderCompletionStatus(net::OK));
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, Invalid_CertMessage) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();

  const std::string message = "Invalid cert message";

  mojo::DataPipe data_pipe(message.size());
  CHECK(
      mojo::common::BlockingCopyFromString(message, data_pipe.producer_handle));
  data_pipe.producer_handle.reset();
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));

  mock_loader_factory_.client_ptr()->OnComplete(
      network::URLLoaderCompletionStatus(net::OK));
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, DeleteFetcher_BeforeReceiveResponse) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  RunUntilIdle();
  fetcher.reset();
  RunUntilIdle();

  EXPECT_FALSE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, DeleteFetcher_BeforeResponseBody) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  RunUntilIdle();
  fetcher.reset();
  RunUntilIdle();

  EXPECT_FALSE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, DeleteFetcher_WhileReceivingBody) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
  const std::string message =
      CreateCertMessage(CreateCertMessageFromCert(*certificate));
  mojo::DataPipe data_pipe(message.size() / 2 + 1);
  CHECK(mojo::common::BlockingCopyFromString(
      message.substr(0, message.size() / 2), data_pipe.producer_handle));
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  RunUntilIdle();
  fetcher.reset();
  RunUntilIdle();
  CHECK(mojo::common::BlockingCopyFromString(message.substr(message.size() / 2),
                                             data_pipe.producer_handle));
  RunUntilIdle();

  EXPECT_FALSE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, DeleteFetcher_AfterReceivingBody) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
  const std::string message =
      CreateCertMessage(CreateCertMessageFromCert(*certificate));
  mojo::DataPipe data_pipe(message.size());
  CHECK(
      mojo::common::BlockingCopyFromString(message, data_pipe.producer_handle));
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  RunUntilIdle();
  CloseClientPipe();
  RunUntilIdle();
  data_pipe.producer_handle.reset();
  fetcher.reset();
  RunUntilIdle();

  EXPECT_FALSE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, CloseClientPipe_BeforeReceiveResponse) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  RunUntilIdle();
  CloseClientPipe();
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, CloseClientPipe_BeforeResponseBody) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  RunUntilIdle();
  CloseClientPipe();
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, CloseClientPipe_WhileReceivingBody) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
  const std::string message =
      CreateCertMessage(CreateCertMessageFromCert(*certificate));
  mojo::DataPipe data_pipe(message.size() / 2 + 1);
  CHECK(mojo::common::BlockingCopyFromString(
      message.substr(0, message.size() / 2), data_pipe.producer_handle));
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  RunUntilIdle();
  CloseClientPipe();
  RunUntilIdle();
  data_pipe.producer_handle.reset();
  RunUntilIdle();
  EXPECT_TRUE(callback_called_);
  EXPECT_FALSE(cert_result_);
}

TEST_F(SignedExchangeCertFetcherTest, CloseClientPipe_AfterReceivingBody) {
  std::unique_ptr<SignedExchangeCertFetcher> fetcher =
      CreateFetcherAndStart(false /* force_fetch */);
  CallOnReceiveResponse();
  scoped_refptr<net::X509Certificate> certificate = ImportTestCert();
  const std::string message =
      CreateCertMessage(CreateCertMessageFromCert(*certificate));
  mojo::DataPipe data_pipe(message.size());
  CHECK(
      mojo::common::BlockingCopyFromString(message, data_pipe.producer_handle));
  mock_loader_factory_.client_ptr()->OnStartLoadingResponseBody(
      std::move(data_pipe.consumer_handle));
  RunUntilIdle();
  CloseClientPipe();
  RunUntilIdle();
  data_pipe.producer_handle.reset();
  RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  ASSERT_TRUE(cert_result_);
  EXPECT_EQ(certificate->CalculateChainFingerprint256(),
            cert_result_->CalculateChainFingerprint256());
}

}  // namespace content
