// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <list>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/network/network_context.h"
#include "content/network/url_loader_impl.h"
#include "content/public/common/appcache_info.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/test_url_loader_client.h"
#include "mojo/public/c/system/data_pipe.h"
#include "mojo/public/cpp/system/wait.h"
#include "net/base/io_buffer.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

static ResourceRequest CreateResourceRequest(const char* method,
                                             ResourceType type,
                                             const GURL& url) {
  ResourceRequest request;
  request.method = std::string(method);
  request.url = url;
  request.first_party_for_cookies = url;  // bypass third-party cookie blocking
  request.request_initiator = url::Origin(url);  // ensure initiator is set
  request.referrer_policy = blink::kWebReferrerPolicyDefault;
  request.load_flags = 0;
  request.origin_pid = 0;
  request.resource_type = type;
  request.request_context = 0;
  request.appcache_host_id = kAppCacheNoHostId;
  request.download_to_file = false;
  request.should_reset_appcache = false;
  request.is_main_frame = true;
  request.parent_is_main_frame = false;
  request.transition_type = ui::PAGE_TRANSITION_LINK;
  request.allow_download = true;
  return request;
}

class URLRequestSlowWriterJob : public net::URLRequestJob {
 public:
  URLRequestSlowWriterJob(net::URLRequest* request,
                          net::NetworkDelegate* network_delegate,
                          std::list<std::string> packets)
      : URLRequestJob(request, network_delegate),
        packets_(std::move(packets)) {}

  // net::URLRequestJob implementation:
  void Start() override { NotifyHeadersComplete(); }
  int ReadRawData(net::IOBuffer* buf, int buf_size) override {
    if (packets_.empty())
      return 0;

    std::string packet = packets_.front();
    packets_.pop_front();
    CHECK_GE(buf_size, static_cast<int>(packet.length()));
    memcpy(buf->data(), packet.c_str(), packet.length());
    return packet.length();
  }

 private:
  ~URLRequestSlowWriterJob() override {}

  std::list<std::string> packets_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestSlowWriterJob);
};

class SlowWriterInterceptor : public net::URLRequestInterceptor {
 public:
  explicit SlowWriterInterceptor(std::list<std::string> packets)
      : packets_(std::move(packets)) {}
  ~SlowWriterInterceptor() override {}

  static GURL GetURL() { return GURL("http://foo"); }

  // URLRequestInterceptor implementation:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    return new URLRequestSlowWriterJob(request, network_delegate,
                                       std::move(packets_));
  }

 private:
  std::list<std::string> packets_;
  DISALLOW_COPY_AND_ASSIGN(SlowWriterInterceptor);
};

}  // namespace

class URLLoaderImplTest : public testing::Test {
 public:
  URLLoaderImplTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        context_(NetworkContext::CreateForTesting()) {}
  ~URLLoaderImplTest() override {}

  void SetUp() override {
    test_server_.AddDefaultHandlers(
        base::FilePath(FILE_PATH_LITERAL("content/test/data")));
    ASSERT_TRUE(test_server_.Start());
  }

  void Load(const GURL& url) {
    mojom::URLLoaderPtr loader;

    ResourceRequest request =
        CreateResourceRequest("GET", RESOURCE_TYPE_MAIN_FRAME, url);
    uint32_t options = mojom::kURLLoadOptionNone;
    if (send_ssl_)
      options |= mojom::kURLLoadOptionSendSSLInfo;
    if (sniff_)
      options |= mojom::kURLLoadOptionSniffMimeType;

    URLLoaderImpl loader_impl(context(), mojo::MakeRequest(&loader), options,
                              request, client_.CreateInterfacePtr(),
                              TRAFFIC_ANNOTATION_FOR_TESTS);

    client_.RunUntilComplete();
    ran_ = true;
  }

  void LoadAndCompareFile(const std::string& path) {
    base::FilePath file;
    PathService::Get(content::DIR_TEST_DATA, &file);
    file = file.AppendASCII(path);

    std::string expected;
    if (!base::ReadFileToString(file, &expected)) {
      ADD_FAILURE() << "File not found: " << file.value();
      return;
    }

    Load(test_server()->GetURL(std::string("/") + path));
    CHECK_EQ(expected, ReadData(expected.size()));
  }

  void LoadPackets(std::list<std::string> packets, bool sniff = true) {
    net::URLRequestFilter::GetInstance()->AddUrlInterceptor(
        SlowWriterInterceptor::GetURL(),
        std::unique_ptr<net::URLRequestInterceptor>(
            new SlowWriterInterceptor(std::move(packets))));
    Load(SlowWriterInterceptor::GetURL());
    net::URLRequestFilter::GetInstance()->ClearHandlers();
  }

  net::EmbeddedTestServer* test_server() { return &test_server_; }
  NetworkContext* context() { return context_.get(); }
  TestURLLoaderClient* client() { return &client_; }
  void DestroyContext() { context_.reset(); }

  // Configure how Load() works.
  void set_sniff() {
    DCHECK(!ran_);
    sniff_ = true;
  }
  void set_send_ssl() {
    DCHECK(!ran_);
    send_ssl_ = true;
  }

  // Convenience methods after calling Load();
  std::string mime_type() {
    DCHECK(ran_);
    return client_.response_head().mime_type;
  }
  const base::Optional<net::SSLInfo>& ssl_info() {
    DCHECK(ran_);
    return client()->ssl_info();
  }

  std::string ReadData(size_t size) {
    DCHECK(ran_);
    MojoHandle consumer = client()->response_body().value();
    MojoResult rv =
        mojo::Wait(mojo::Handle(consumer), MOJO_HANDLE_SIGNAL_READABLE);
    if (!size) {
      CHECK_EQ(rv, MOJO_RESULT_FAILED_PRECONDITION);
      return std::string();
    }
    CHECK_EQ(rv, MOJO_RESULT_OK);
    std::vector<char> buffer(size);
    uint32_t num_bytes = static_cast<uint32_t>(size);
    CHECK_EQ(MojoReadData(consumer, buffer.data(), &num_bytes,
                          MOJO_READ_DATA_FLAG_ALL_OR_NONE),
             MOJO_RESULT_OK);
    CHECK_EQ(num_bytes, static_cast<uint32_t>(size));

    return std::string(buffer.data(), buffer.size());
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  net::EmbeddedTestServer test_server_;
  std::unique_ptr<NetworkContext> context_;
  bool sniff_ = false;
  bool send_ssl_ = false;
  bool ran_ = false;
  TestURLLoaderClient client_;
};

TEST_F(URLLoaderImplTest, Basic) {
  LoadAndCompareFile("simple_page.html");
}

TEST_F(URLLoaderImplTest, Empty) {
  LoadAndCompareFile("empty.html");
}

TEST_F(URLLoaderImplTest, BasicSSL) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.ServeFilesFromSourceDirectory(
      base::FilePath(FILE_PATH_LITERAL("content/test/data")));
  ASSERT_TRUE(https_server.Start());

  GURL url = https_server.GetURL("/simple_page.html");
  set_send_ssl();
  Load(url);
  ASSERT_TRUE(!!ssl_info());
  ASSERT_TRUE(!!ssl_info()->cert);

  ASSERT_TRUE(https_server.GetCertificate()->Equals(ssl_info()->cert.get()));
}

TEST_F(URLLoaderImplTest, SSLSentOnlyWhenRequested) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.ServeFilesFromSourceDirectory(
      base::FilePath(FILE_PATH_LITERAL("content/test/data")));
  ASSERT_TRUE(https_server.Start());

  GURL url = https_server.GetURL("/simple_page.html");
  Load(url);
  ASSERT_FALSE(!!ssl_info());
}

TEST_F(URLLoaderImplTest, DestroyContextWithLiveRequest) {
  GURL url = test_server()->GetURL("/hung-after-headers");
  ResourceRequest request =
      CreateResourceRequest("GET", RESOURCE_TYPE_MAIN_FRAME, url);

  mojom::URLLoaderPtr loader;
  // The loader is implicitly owned by the client and the NetworkContext, so
  // don't hold on to a pointer to it.
  base::WeakPtr<URLLoaderImpl> loader_impl =
      (new URLLoaderImpl(context(), mojo::MakeRequest(&loader), 0, request,
                         client()->CreateInterfacePtr(),
                         TRAFFIC_ANNOTATION_FOR_TESTS))
          ->GetWeakPtrForTests();

  client()->RunUntilResponseReceived();
  EXPECT_TRUE(client()->has_received_response());
  EXPECT_FALSE(client()->has_received_completion());

  // Request hasn't completed, so the loader should not have been destroyed.
  EXPECT_TRUE(loader_impl);

  // Destroying the context should result in destroying the loader and the
  // client receiving a connection error.
  DestroyContext();
  EXPECT_FALSE(loader_impl);

  client()->RunUntilConnectionError();
  EXPECT_FALSE(client()->has_received_completion());
  EXPECT_EQ(0u, client()->download_data_length());
}

TEST_F(URLLoaderImplTest, DoNotSniffUnlessSpecified) {
  Load(test_server()->GetURL("/content-sniffer-test0.html"));
  ASSERT_TRUE(mime_type().empty());
}

TEST_F(URLLoaderImplTest, SniffMimeType) {
  set_sniff();
  Load(test_server()->GetURL("/content-sniffer-test0.html"));
  ASSERT_EQ(std::string("text/html"), mime_type());
}

TEST_F(URLLoaderImplTest, RespectNoSniff) {
  set_sniff();
  Load(test_server()->GetURL("/nosniff-test.html"));
  ASSERT_TRUE(mime_type().empty());
}

TEST_F(URLLoaderImplTest, DoNotSniffHTMLFromTextPlain) {
  set_sniff();
  Load(test_server()->GetURL("/content-sniffer-test1.html"));
  ASSERT_EQ(std::string("text/plain"), mime_type());
}

TEST_F(URLLoaderImplTest, DoNotSniffHTMLFromImageGIF) {
  set_sniff();
  Load(test_server()->GetURL("/content-sniffer-test2.html"));
  ASSERT_EQ(std::string("image/gif"), mime_type());
}

TEST_F(URLLoaderImplTest, CantSniffEmptyHtml) {
  set_sniff();
  Load(test_server()->GetURL("/content-sniffer-test4.html"));
  ASSERT_TRUE(mime_type().empty());
}

TEST_F(URLLoaderImplTest, FirstReadNotEnoughToSniff) {
  std::string first(500, 'a');
  std::string second(std::string(100, 'b'));
  second[10] = 0;
  std::list<std::string> packets;
  packets.push_back(first);
  packets.push_back(second);
  set_sniff();
  LoadPackets(std::move(packets));
  ASSERT_EQ(std::string("application/octet-stream"), mime_type());
  std::string expected = first + second;
  CHECK_EQ(expected, ReadData(expected.size()));
}

TEST_F(URLLoaderImplTest, LoneReadNotEnoughToSniff) {
  std::string first(500, 'a');
  std::list<std::string> packets;
  packets.push_back(first);
  set_sniff();
  LoadPackets(std::move(packets));
  ASSERT_EQ(std::string("text/plain"), mime_type());
  CHECK_EQ(first, ReadData(first.size()));
}

}  // namespace content
