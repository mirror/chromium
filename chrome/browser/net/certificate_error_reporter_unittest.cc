// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/certificate_error_reporter.h"

#include <set>
#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/thread_task_runner_handle.h"
#include "chrome/browser/net/cert_logger.pb.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/network_delegate_impl.h"
#include "net/base/test_data_directory.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_element_reader.h"
#include "net/test/cert_test_util.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "net/test/url_request/url_request_mock_data_job.h"
#include "net/test/url_request/url_request_mock_http_job.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using chrome_browser_net::CertificateErrorReporter;
using content::BrowserThread;
using net::CompletionCallback;
using net::SSLInfo;
using net::NetworkDelegateImpl;
using net::TestURLRequestContext;
using net::URLRequest;

namespace {

const char kHostname[] = "test.mail.google.com";
const char kSecondRequestHostname[] = "test2.mail.google.com";
const char kDummyFailureLog[] = "dummy failure log";
const char kTestCertFilename[] = "test_mail_google_com.pem";

SSLInfo GetTestSSLInfo() {
  SSLInfo info;
  info.cert =
      net::ImportCertFromFile(net::GetTestCertsDirectory(), kTestCertFilename);
  info.is_issued_by_known_root = true;
  info.pinning_failure_log = kDummyFailureLog;
  return info;
}

std::string GetPEMEncodedChain() {
  base::FilePath cert_path =
      net::GetTestCertsDirectory().AppendASCII(kTestCertFilename);
  std::string cert_data;
  EXPECT_TRUE(base::ReadFileToString(cert_path, &cert_data));
  return cert_data;
}

void EnableUrlRequestMocks(bool enable) {
  net::URLRequestFilter::GetInstance()->ClearHandlers();
  if (!enable)
    return;

  base::FilePath root_http;
  net::URLRequestFailedJob::AddUrlHandler();
  PathService::Get(chrome::DIR_TEST_DATA, &root_http);
  net::URLRequestMockHTTPJob::AddUrlHandlers(root_http,
                                             BrowserThread::GetBlockingPool());
  net::URLRequestMockDataJob::AddUrlHandler();
}

// Check that data uploaded in the request matches the test data (an SSL
// report for one of the given hostnames, with the info returned by
// |GetTestSSLInfo()|). The hostname sent in the report will be erased
// from |expected_hostnames|.
void CheckUploadData(URLRequest* request,
                     std::set<std::string>* expected_hostnames) {
  const net::UploadDataStream* upload = request->get_upload();
  ASSERT_TRUE(upload);
  ASSERT_TRUE(upload->GetElementReaders());
  EXPECT_EQ(1u, upload->GetElementReaders()->size());

  const net::UploadBytesElementReader* reader =
      (*upload->GetElementReaders())[0]->AsBytesReader();
  ASSERT_TRUE(reader);
  std::string upload_data(reader->bytes(), reader->length());
  chrome_browser_net::CertLoggerRequest uploaded_request;

  uploaded_request.ParseFromString(upload_data);

  EXPECT_EQ(1u, expected_hostnames->count(uploaded_request.hostname()));
  expected_hostnames->erase(uploaded_request.hostname());

  EXPECT_EQ(GetPEMEncodedChain(), uploaded_request.cert_chain());
  EXPECT_EQ(1, uploaded_request.pin().size());
  EXPECT_EQ(kDummyFailureLog, uploaded_request.pin().Get(0));
}

// A network delegate that lets tests check that a certificate error
// report was sent. It counts the number of requests and lets tests
// register a callback to run when the request is destroyed. It also
// checks that the uploaded data is as expected (a report for
// |kHostname| and |GetTestSSLInfo()|).
class TestCertificateErrorReporterNetworkDelegate : public NetworkDelegateImpl {
 public:
  TestCertificateErrorReporterNetworkDelegate()
      : url_request_destroyed_callback_(base::Bind(&base::DoNothing)),
        all_url_requests_destroyed_callback_(base::Bind(&base::DoNothing)),
        num_requests_(0) {}

  ~TestCertificateErrorReporterNetworkDelegate() override {}

  int OnBeforeURLRequest(URLRequest* request,
                         const CompletionCallback& callback,
                         GURL* new_url) override {
    num_requests_++;
    EXPECT_EQ(expected_url_, request->url());
    EXPECT_EQ("POST", request->method());

    std::string uploaded_request_hostname;
    CheckUploadData(request, &expected_hostnames_);
    expected_hostnames_.erase(uploaded_request_hostname);
    return net::OK;
  }

  void OnURLRequestDestroyed(URLRequest* request) override {
    url_request_destroyed_callback_.Run();

    if (expected_hostnames_.empty())
      all_url_requests_destroyed_callback_.Run();
  }

  void ExpectHostname(const std::string& hostname) {
    expected_hostnames_.insert(hostname);
  }

  void set_all_url_requests_destroyed_callback(
      const base::Closure& all_url_requests_destroyed_callback) {
    all_url_requests_destroyed_callback_ = all_url_requests_destroyed_callback;
  }

  void set_url_request_destroyed_callback(
      const base::Closure& url_request_destroyed_callback) {
    url_request_destroyed_callback_ = url_request_destroyed_callback;
  }

  void set_expected_url(const GURL& expected_url) {
    expected_url_ = expected_url;
  }

  int num_requests() const { return num_requests_; }

 private:
  base::Closure url_request_destroyed_callback_;
  base::Closure all_url_requests_destroyed_callback_;
  int num_requests_;
  GURL expected_url_;
  std::set<std::string> expected_hostnames_;

  DISALLOW_COPY_AND_ASSIGN(TestCertificateErrorReporterNetworkDelegate);
};

class CertificateErrorReporterTest : public ::testing::Test {
 public:
  CertificateErrorReporterTest() : context_(true) {
    EnableUrlRequestMocks(true);
    context_.set_network_delegate(&network_delegate_);
    context_.Init();
  }

  ~CertificateErrorReporterTest() override { EnableUrlRequestMocks(false); }

  TestCertificateErrorReporterNetworkDelegate* network_delegate() {
    return &network_delegate_;
  }

  TestURLRequestContext* context() { return &context_; }

 private:
  base::MessageLoop message_loop_;
  TestCertificateErrorReporterNetworkDelegate network_delegate_;
  TestURLRequestContext context_;
};

void SendReport(TestCertificateErrorReporterNetworkDelegate* network_delegate,
                TestURLRequestContext* context,
                const std::string& report_hostname,
                const GURL& url,
                int request_sequence_number) {
  base::RunLoop run_loop;
  network_delegate->set_url_request_destroyed_callback(run_loop.QuitClosure());

  network_delegate->set_expected_url(url);
  network_delegate->ExpectHostname(report_hostname);

  CertificateErrorReporter reporter(
      context, url, CertificateErrorReporter::DO_NOT_SEND_COOKIES);

  EXPECT_EQ(request_sequence_number, network_delegate->num_requests());

  reporter.SendReport(CertificateErrorReporter::REPORT_TYPE_PINNING_VIOLATION,
                      report_hostname, GetTestSSLInfo());
  run_loop.Run();

  EXPECT_EQ(request_sequence_number + 1, network_delegate->num_requests());
}

// Test that CertificateErrorReporter::SendReport creates a URLRequest
// for the endpoint and sends the expected data.
TEST_F(CertificateErrorReporterTest, SendReportSendsRequest) {
  GURL url = net::URLRequestMockDataJob::GetMockHttpsUrl("dummy data", 1);
  SendReport(network_delegate(), context(), kHostname, url, 0);
}

TEST_F(CertificateErrorReporterTest, SendMultipleReportsSequentially) {
  GURL url = net::URLRequestMockDataJob::GetMockHttpsUrl("dummy data", 1);
  SendReport(network_delegate(), context(), kHostname, url, 0);
  SendReport(network_delegate(), context(), kSecondRequestHostname, url, 1);
}

TEST_F(CertificateErrorReporterTest, SendMultipleReportsSimultaneously) {
  base::RunLoop run_loop;
  network_delegate()->set_all_url_requests_destroyed_callback(
      run_loop.QuitClosure());

  GURL url = net::URLRequestMockDataJob::GetMockHttpsUrl("dummy data", 1);
  network_delegate()->set_expected_url(url);
  network_delegate()->ExpectHostname(kHostname);
  network_delegate()->ExpectHostname(kSecondRequestHostname);

  CertificateErrorReporter reporter(
      context(), url, CertificateErrorReporter::DO_NOT_SEND_COOKIES);

  EXPECT_EQ(0, network_delegate()->num_requests());

  reporter.SendReport(CertificateErrorReporter::REPORT_TYPE_PINNING_VIOLATION,
                      kHostname, GetTestSSLInfo());
  reporter.SendReport(CertificateErrorReporter::REPORT_TYPE_PINNING_VIOLATION,
                      kSecondRequestHostname, GetTestSSLInfo());

  run_loop.Run();

  EXPECT_EQ(2, network_delegate()->num_requests());
}

// Test that pending URLRequests get cleaned up when the reporter is
// deleted.
TEST_F(CertificateErrorReporterTest, PendingRequestGetsDeleted) {
  base::RunLoop run_loop;
  network_delegate()->set_url_request_destroyed_callback(
      run_loop.QuitClosure());

  GURL url = net::URLRequestMockHTTPJob::GetMockUrlWithFailure(
      base::FilePath(FILE_PATH_LITERAL("empty.html")),
      net::URLRequestMockHTTPJob::START, net::ERR_IO_PENDING);
  network_delegate()->set_expected_url(url);
  network_delegate()->ExpectHostname(kHostname);

  EXPECT_EQ(0, network_delegate()->num_requests());

  scoped_ptr<CertificateErrorReporter> reporter(new CertificateErrorReporter(
      context(), url, CertificateErrorReporter::DO_NOT_SEND_COOKIES));
  reporter->SendReport(CertificateErrorReporter::REPORT_TYPE_PINNING_VIOLATION,
                       kHostname, GetTestSSLInfo());
  reporter.reset();

  run_loop.Run();

  EXPECT_EQ(1, network_delegate()->num_requests());
}

// Test that a request that returns an error gets cleaned up.
TEST_F(CertificateErrorReporterTest, ErroredRequestGetsDeleted) {
  GURL url = net::URLRequestFailedJob::GetMockHttpsUrl(net::ERR_FAILED);
  SendReport(network_delegate(), context(), kHostname, url, 0);
}

}  // namespace
