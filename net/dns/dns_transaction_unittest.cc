// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/dns_transaction.h"

#include <stdint.h>

#include <limits>
#include <utility>

#include "base/base64url.h"
#include "base/bind.h"
#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/sys_byteorder.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "net/base/ip_address.h"
#include "net/dns/dns_protocol.h"
#include "net/dns/dns_query.h"
#include "net/dns/dns_response.h"
#include "net/dns/dns_session.h"
#include "net/dns/dns_test_util.h"
#include "net/dns/dns_util.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/socket/socket_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/gtest_util.h"
#include "net/test/net_test_suite.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::test::IsOk;

namespace net {

namespace {

base::TimeDelta kTimeout = base::TimeDelta::FromSeconds(1);

std::string DomainFromDot(const base::StringPiece& dotted) {
  std::string out;
  EXPECT_TRUE(DNSDomainFromDot(dotted, &out));
  return out;
}

enum Transport { UDP, TCP, HTTPS };

static inline bool use_tcp(Transport t) {
  return t == TCP;
}

// A SocketDataProvider builder.
class DnsSocketData {
 public:
  // The ctor takes parameters for the DnsQuery.
  DnsSocketData(uint16_t id,
                const char* dotted_name,
                uint16_t qtype,
                IoMode mode,
                Transport trans,
                const OptRecordRdata* opt_rdata = nullptr)
      : query_(new DnsQuery(id, DomainFromDot(dotted_name), qtype, opt_rdata)),
        trans_(trans) {
    if (use_tcp(trans_)) {
      std::unique_ptr<uint16_t> length(new uint16_t);
      *length = base::HostToNet16(query_->io_buffer()->size());
      writes_.push_back(MockWrite(mode,
                                  reinterpret_cast<const char*>(length.get()),
                                  sizeof(uint16_t), num_reads_and_writes()));
      lengths_.push_back(std::move(length));
    }
    writes_.push_back(MockWrite(mode, query_->io_buffer()->data(),
                                query_->io_buffer()->size(),
                                num_reads_and_writes()));
  }
  ~DnsSocketData() = default;

  // All responses must be added before GetProvider.

  // Adds pre-built DnsResponse. |tcp_length| will be used in TCP mode only.
  void AddResponseWithLength(std::unique_ptr<DnsResponse> response,
                             IoMode mode,
                             uint16_t tcp_length) {
    CHECK(!provider_.get());
    if (use_tcp(trans_)) {
      std::unique_ptr<uint16_t> length(new uint16_t);
      *length = base::HostToNet16(tcp_length);
      reads_.push_back(MockRead(mode,
                                reinterpret_cast<const char*>(length.get()),
                                sizeof(uint16_t), num_reads_and_writes()));
      lengths_.push_back(std::move(length));
    }
    reads_.push_back(MockRead(mode, response->io_buffer()->data(),
                              response->io_buffer_size(),
                              num_reads_and_writes()));
    responses_.push_back(std::move(response));
  }

  // Adds pre-built DnsResponse.
  void AddResponse(std::unique_ptr<DnsResponse> response, IoMode mode) {
    uint16_t tcp_length = response->io_buffer_size();
    AddResponseWithLength(std::move(response), mode, tcp_length);
  }

  // Adds pre-built response from |data| buffer.
  void AddResponseData(const uint8_t* data, size_t length, IoMode mode) {
    CHECK(!provider_.get());
    AddResponse(std::make_unique<DnsResponse>(
                    reinterpret_cast<const char*>(data), length, 0),
                mode);
  }

  // Add no-answer (RCODE only) response matching the query.
  void AddRcode(int rcode, IoMode mode) {
    std::unique_ptr<DnsResponse> response(new DnsResponse(
        query_->io_buffer()->data(), query_->io_buffer()->size(), 0));
    dns_protocol::Header* header =
        reinterpret_cast<dns_protocol::Header*>(response->io_buffer()->data());
    header->flags |= base::HostToNet16(dns_protocol::kFlagResponse | rcode);
    AddResponse(std::move(response), mode);
  }

  // Add error response.
  void AddReadError(int error, IoMode mode) {
    reads_.push_back(MockRead(mode, error, num_reads_and_writes()));
  }

  // Build, if needed, and return the SocketDataProvider. No new responses
  // should be added afterwards.
  SequencedSocketData* GetProvider() {
    if (provider_.get())
      return provider_.get();
    // Terminate the reads with ERR_IO_PENDING to prevent overrun and default to
    // timeout.
    reads_.push_back(
        MockRead(SYNCHRONOUS, ERR_IO_PENDING, writes_.size() + reads_.size()));
    provider_.reset(new SequencedSocketData(&reads_[0], reads_.size(),
                                            &writes_[0], writes_.size()));
    if (use_tcp(trans_)) {
      provider_->set_connect_data(MockConnect(reads_[0].mode, OK));
    }
    return provider_.get();
  }

  std::string GetData() {
    std::string data;
    for (auto w : writes_) {
      provider_->OnWrite(std::string(w.data, w.data_len));
      base::RunLoop().RunUntilIdle();
    }
    for (auto r : reads_) {
      data.append(r.data, r.data_len);
    }
    return data;
  }

  uint16_t query_id() const { return query_->id(); }

  IOBufferWithSize* query_buffer() { return query_->io_buffer(); }

  Transport get_trans() { return trans_; }

 private:
  size_t num_reads_and_writes() const { return reads_.size() + writes_.size(); }

  std::unique_ptr<DnsQuery> query_;
  Transport trans_;
  std::vector<std::unique_ptr<uint16_t>> lengths_;
  std::vector<std::unique_ptr<DnsResponse>> responses_;
  std::vector<MockWrite> writes_;
  std::vector<MockRead> reads_;
  std::unique_ptr<SequencedSocketData> provider_;

  DISALLOW_COPY_AND_ASSIGN(DnsSocketData);
};

class TestSocketFactory;

// A variant of MockUDPClientSocket which always fails to Connect.
class FailingUDPClientSocket : public MockUDPClientSocket {
 public:
  FailingUDPClientSocket(SocketDataProvider* data,
                         net::NetLog* net_log)
      : MockUDPClientSocket(data, net_log) {
  }
  ~FailingUDPClientSocket() override = default;
  int Connect(const IPEndPoint& endpoint) override {
    return ERR_CONNECTION_REFUSED;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FailingUDPClientSocket);
};

// A variant of MockUDPClientSocket which notifies the factory OnConnect.
class TestUDPClientSocket : public MockUDPClientSocket {
 public:
  TestUDPClientSocket(TestSocketFactory* factory,
                      SocketDataProvider* data,
                      net::NetLog* net_log)
      : MockUDPClientSocket(data, net_log), factory_(factory) {
  }
  ~TestUDPClientSocket() override = default;
  int Connect(const IPEndPoint& endpoint) override;

 private:
  TestSocketFactory* factory_;

  DISALLOW_COPY_AND_ASSIGN(TestUDPClientSocket);
};

// Creates TestUDPClientSockets and keeps endpoints reported via OnConnect.
class TestSocketFactory : public MockClientSocketFactory {
 public:
  TestSocketFactory() : fail_next_socket_(false) {}
  ~TestSocketFactory() override = default;

  std::unique_ptr<DatagramClientSocket> CreateDatagramClientSocket(
      DatagramSocket::BindType bind_type,
      const RandIntCallback& rand_int_cb,
      NetLog* net_log,
      const NetLogSource& source) override {
    if (fail_next_socket_) {
      fail_next_socket_ = false;
      return std::unique_ptr<DatagramClientSocket>(
          new FailingUDPClientSocket(&empty_data_, net_log));
    }
    SocketDataProvider* data_provider = mock_data().GetNext();
    std::unique_ptr<TestUDPClientSocket> socket(
        new TestUDPClientSocket(this, data_provider, net_log));
    return std::move(socket);
  }

  void OnConnect(const IPEndPoint& endpoint) {
    remote_endpoints_.push_back(endpoint);
  }

  std::vector<IPEndPoint> remote_endpoints_;
  bool fail_next_socket_;

 private:
  StaticSocketDataProvider empty_data_;

  DISALLOW_COPY_AND_ASSIGN(TestSocketFactory);
};

int TestUDPClientSocket::Connect(const IPEndPoint& endpoint) {
  factory_->OnConnect(endpoint);
  return MockUDPClientSocket::Connect(endpoint);
}

// Helper class that holds a DnsTransaction and handles OnTransactionComplete.
class TransactionHelper {
 public:
  // If |expected_answer_count| < 0 then it is the expected net error.
  TransactionHelper(const char* hostname,
                    uint16_t qtype,
                    int expected_answer_count)
      : hostname_(hostname),
        qtype_(qtype),
        expected_answer_count_(expected_answer_count),
        cancel_in_callback_(false),
        completed_(false) {}

  // Mark that the transaction shall be destroyed immediately upon callback.
  void set_cancel_in_callback() {
    cancel_in_callback_ = true;
  }

  void add_doh_server(GURL url, bool use_post) {
    doh_servers.push_back(DnsConfig::DnsOverHttpsServerConfig(url, use_post));
  }

  void StartTransaction(DnsTransactionFactory* factory) {
    EXPECT_EQ(NULL, transaction_.get());
    transaction_ = factory->CreateTransaction(
        hostname_, qtype_, base::Bind(&TransactionHelper::OnTransactionComplete,
                                      base::Unretained(this)),
        NetLogWithSource());
    net::URLRequestContextBuilder builder;
    builder.set_proxy_config_service(
        std::make_unique<net::ProxyConfigServiceFixed>(net::ProxyConfig()));

    transaction_->SetRequestContext(new TrivialURLRequestContextGetter(
        builder.Build().release(),

        base::ThreadTaskRunnerHandle::Get()));
    transaction_->SetRequestPriority(DEFAULT_PRIORITY);
    EXPECT_EQ(hostname_, transaction_->GetHostname());
    EXPECT_EQ(qtype_, transaction_->GetType());
    transaction_->Start();
  }

  void Cancel() {
    ASSERT_TRUE(transaction_.get() != NULL);
    transaction_.reset(NULL);
  }

  void OnTransactionComplete(DnsTransaction* t,
                             int rv,
                             const DnsResponse* response) {
    EXPECT_FALSE(completed_);
    EXPECT_EQ(transaction_.get(), t);

    completed_ = true;

    if (cancel_in_callback_) {
      Cancel();
      return;
    }

    if (expected_answer_count_ >= 0) {
      ASSERT_THAT(rv, IsOk());
      ASSERT_TRUE(response != NULL);
      EXPECT_EQ(static_cast<unsigned>(expected_answer_count_),
                response->answer_count());
      EXPECT_EQ(qtype_, response->qtype());

      DnsRecordParser parser = response->Parser();
      DnsResourceRecord record;
      for (int i = 0; i < expected_answer_count_; ++i) {
        EXPECT_TRUE(parser.ReadRecord(&record));
      }
    } else {
      EXPECT_EQ(expected_answer_count_, rv);
    }
  }

  bool has_completed() const {
    return completed_;
  }

  // Shorthands for commonly used commands.

  bool Run(DnsTransactionFactory* factory) {
    StartTransaction(factory);
    base::RunLoop().RunUntilIdle();
    return has_completed();
  }

  bool RunUntilDone(DnsTransactionFactory* factory) {
    StartTransaction(factory);
    do {
      base::RunLoop().RunUntilIdle();
    } while (!has_completed());
    return has_completed();
  }

  bool FastForwardByTimeout(DnsSession* session,
                            unsigned server_index,
                            int attempt) {
    NetTestSuite::GetScopedTaskEnvironment()->FastForwardBy(
        session->NextTimeout(server_index, attempt));
    return has_completed();
  }

  bool FastForwardAll() {
    NetTestSuite::GetScopedTaskEnvironment()->FastForwardUntilNoTasksRemain();
    return has_completed();
  }

 private:
  std::string hostname_;
  uint16_t qtype_;
  std::unique_ptr<DnsTransaction> transaction_;
  int expected_answer_count_;
  bool cancel_in_callback_;
  std::vector<DnsConfig::DnsOverHttpsServerConfig> doh_servers;

  bool completed_;
};

typedef base::RepeatingCallback<std::unique_ptr<test_server::HttpResponse>(
    const net::test_server::HttpRequest&,
    std::string content)>
    ResponseModifier;

class DnsTransactionTest : public testing::Test {
 public:
  DnsTransactionTest() = default;

  // Generates |nameservers| for DnsConfig.
  void ConfigureNumServers(unsigned num_servers) {
    CHECK_LE(num_servers, 255u);
    config_.nameservers.clear();
    for (unsigned i = 0; i < num_servers; ++i) {
      config_.nameservers.push_back(
          IPEndPoint(IPAddress(192, 168, 1, i), dns_protocol::kDefaultPort));
    }
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    std::string decoded_query;
    if (request.method == test_server::HttpMethod::METHOD_GET) {
      std::string request_query(request.GetURL().query());
      int pos = 0;
      std::string encoded_query;
      while (pos >= 0) {
        int eq = request_query.find("=", pos);
        if (request_query.substr(pos, eq - pos) == "body") {
          int end = request_query.find("&", eq);
          if (end == -1) {
            end = request_query.length();
          }
          encoded_query = request_query.substr(eq + 1, end - eq);
          break;
        }
        pos = request_query.find("&", pos);
        if (pos > 0)
          pos++;
      }
      CHECK_GT(encoded_query.size(), 0ul);

      CHECK_EQ(true,
               base::Base64UrlDecode(
                   encoded_query, base::Base64UrlDecodePolicy::IGNORE_PADDING,
                   &decoded_query));
    } else if (request.method == test_server::HttpMethod::METHOD_POST) {
      decoded_query = request.content;
    }
    std::string content;
    for (unsigned int i = 0; i < socket_data_.size(); i++) {
      if (socket_data_[i]->get_trans() != HTTPS)
        continue;
      base::StringPiece query(socket_data_[i]->query_buffer()->data(),
                              socket_data_[i]->query_buffer()->size());
      if (query == decoded_query)
        content.append(socket_data_[i]->GetData());
    }
    doh_responses_served_++;
    return response_modifier_.Run(request, content);
  }

  static std::unique_ptr<test_server::HttpResponse> MakeResponse(
      const net::test_server::HttpRequest&,
      std::string content) {
    std::unique_ptr<test_server::BasicHttpResponse> resp =
        std::make_unique<test_server::BasicHttpResponse>();
    resp->set_content(content);
    resp->set_content_type("application/dns-udpwireformat");
    return resp;
  }

  void ConfigDoHServers(
      bool clearUDP,
      bool use_post,
      TransactionHelper* helper,
      ResponseModifier response_modifier =
          base::BindRepeating(&DnsTransactionTest::MakeResponse)) {
    response_modifier_ = response_modifier;
    if (clearUDP) {
      config_.nameservers.clear();
    }
    NetTestSuite::SetScopedTaskEnvironment(
        base::test::ScopedTaskEnvironment::MainThreadType::IO);
    doh_responses_served_ = 0;
    doh_server_ =
        std::make_unique<EmbeddedTestServer>(EmbeddedTestServer::TYPE_HTTPS);

    doh_server_->RegisterRequestHandler(
        base::Bind(&DnsTransactionTest::HandleRequest, base::Unretained(this)));

    ASSERT_TRUE(doh_server_->Start());
    GURL url = doh_server_->GetURL(std::string("/doh_test"));
    config_.dns_over_https_servers_ = {
        DnsConfig::DnsOverHttpsServerConfig(url, use_post)};
    ConfigureFactory();
  }

  void ShutdownDohServers() {
    printf("shutting down\n");
    ASSERT_TRUE(doh_server_->ShutdownAndWaitUntilComplete());
  }

  // Called after fully configuring |config|.
  void ConfigureFactory() {
    socket_factory_.reset(new TestSocketFactory());
    session_ = new DnsSession(
        config_, DnsSocketPool::CreateNull(socket_factory_.get(),
                                           base::Bind(base::RandInt)),
        base::Bind(&DnsTransactionTest::GetNextId, base::Unretained(this)),
        NULL /* NetLog */);
    transaction_factory_ = DnsTransactionFactory::CreateFactory(session_.get());
  }

  void AddSocketData(std::unique_ptr<DnsSocketData> data) {
    CHECK(socket_factory_.get());
    transaction_ids_.push_back(data->query_id());
    socket_factory_->AddSocketDataProvider(data->GetProvider());
    socket_data_.push_back(std::move(data));
  }

  // Add expected query for |dotted_name| and |qtype| with |id| and response
  // taken verbatim from |data| of |data_length| bytes. The transaction id in
  // |data| should equal |id|, unless testing mismatched response.
  void AddQueryAndResponse(uint16_t id,
                           const char* dotted_name,
                           uint16_t qtype,
                           const uint8_t* response_data,
                           size_t response_length,
                           IoMode mode,
                           Transport trans,
                           const OptRecordRdata* opt_rdata = nullptr) {
    CHECK(socket_factory_.get());
    std::unique_ptr<DnsSocketData> data(
        new DnsSocketData(id, dotted_name, qtype, mode, trans, opt_rdata));
    data->AddResponseData(response_data, response_length, mode);
    AddSocketData(std::move(data));
  }

  void AddAsyncQueryAndResponse(uint16_t id,
                                const char* dotted_name,
                                uint16_t qtype,
                                const uint8_t* data,
                                size_t data_length,
                                const OptRecordRdata* opt_rdata = nullptr) {
    AddQueryAndResponse(id, dotted_name, qtype, data, data_length, ASYNC, UDP,
                        opt_rdata);
  }

  void AddSyncQueryAndResponse(uint16_t id,
                               const char* dotted_name,
                               uint16_t qtype,
                               const uint8_t* data,
                               size_t data_length,
                               const OptRecordRdata* opt_rdata = nullptr) {
    AddQueryAndResponse(id, dotted_name, qtype, data, data_length, SYNCHRONOUS,
                        UDP, opt_rdata);
  }

  // Add expected query of |dotted_name| and |qtype| and no response.
  void AddQueryAndTimeout(const char* dotted_name, uint16_t qtype) {
    uint16_t id = base::RandInt(0, std::numeric_limits<uint16_t>::max());
    std::unique_ptr<DnsSocketData> data(
        new DnsSocketData(id, dotted_name, qtype, ASYNC, UDP));
    AddSocketData(std::move(data));
  }

  // Add expected query of |dotted_name| and |qtype| and matching response with
  // no answer and RCODE set to |rcode|. The id will be generated randomly.
  void AddQueryAndRcode(const char* dotted_name,
                        uint16_t qtype,
                        int rcode,
                        IoMode mode,
                        Transport trans) {
    CHECK_NE(dns_protocol::kRcodeNOERROR, rcode);
    uint16_t id = base::RandInt(0, std::numeric_limits<uint16_t>::max());
    std::unique_ptr<DnsSocketData> data(
        new DnsSocketData(id, dotted_name, qtype, mode, trans));
    data->AddRcode(rcode, mode);
    AddSocketData(std::move(data));
  }

  void AddAsyncQueryAndRcode(const char* dotted_name,
                             uint16_t qtype,
                             int rcode) {
    AddQueryAndRcode(dotted_name, qtype, rcode, ASYNC, UDP);
  }

  void AddSyncQueryAndRcode(const char* dotted_name,
                            uint16_t qtype,
                            int rcode) {
    AddQueryAndRcode(dotted_name, qtype, rcode, SYNCHRONOUS, UDP);
  }

  // Checks if the sockets were connected in the order matching the indices in
  // |servers|.
  void CheckServerOrder(const unsigned* servers, size_t num_attempts) {
    ASSERT_EQ(num_attempts, socket_factory_->remote_endpoints_.size());
    for (size_t i = 0; i < num_attempts; ++i) {
      EXPECT_EQ(socket_factory_->remote_endpoints_[i],
                session_->config().nameservers[servers[i]]);
    }
  }

  void SetUp() override {
    NetTestSuite::SetScopedTaskEnvironment(
        base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME);
    // By default set one server,
    ConfigureNumServers(1);
    // and no retransmissions,
    config_.attempts = 1;
    // and an arbitrary timeout.
    config_.timeout = kTimeout;
    ConfigureFactory();
  }

  void TearDown() override {
    // Check that all socket data was at least written to.
    for (size_t i = 0; i < socket_data_.size(); ++i) {
      EXPECT_TRUE(socket_data_[i]->GetProvider()->AllWriteDataConsumed()) << i;
    }
    NetTestSuite::ResetScopedTaskEnvironment();
  }

 protected:
  int GetNextId(int min, int max) {
    EXPECT_FALSE(transaction_ids_.empty());
    int id = transaction_ids_.front();
    transaction_ids_.pop_front();
    EXPECT_GE(id, min);
    EXPECT_LE(id, max);
    return id;
  }

  DnsConfig config_;

  std::vector<std::unique_ptr<DnsSocketData>> socket_data_;

  base::circular_deque<int> transaction_ids_;
  std::unique_ptr<TestSocketFactory> socket_factory_;
  scoped_refptr<DnsSession> session_;
  std::unique_ptr<DnsTransactionFactory> transaction_factory_;
  std::unique_ptr<EmbeddedTestServer> doh_server_;
  uint32_t doh_responses_served_;
  ResponseModifier response_modifier_;
};

TEST_F(DnsTransactionTest, Lookup) {
  AddAsyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                           kT0ResponseDatagram, arraysize(kT0ResponseDatagram));

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, LookupWithEDNSOption) {
  OptRecordRdata expected_opt_rdata;

  const OptRecordRdata::Opt ednsOpt(123, "\xbe\xef");
  transaction_factory_->AddEDNSOption(ednsOpt);
  expected_opt_rdata.AddOpt(ednsOpt);

  AddAsyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                           kT0ResponseDatagram, arraysize(kT0ResponseDatagram),
                           &expected_opt_rdata);

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, LookupWithMultipleEDNSOptions) {
  OptRecordRdata expected_opt_rdata;

  for (const auto& ednsOpt : {
           // Two options with the same code, to check that both are included.
           OptRecordRdata::Opt(1, "\xde\xad"),
           OptRecordRdata::Opt(1, "\xbe\xef"),
           // Try a different code and different length of data.
           OptRecordRdata::Opt(2, "\xff"),
       }) {
    transaction_factory_->AddEDNSOption(ednsOpt);
    expected_opt_rdata.AddOpt(ednsOpt);
  }

  AddAsyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                           kT0ResponseDatagram, arraysize(kT0ResponseDatagram),
                           &expected_opt_rdata);

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

// Concurrent lookup tests assume that DnsTransaction::Start immediately
// consumes a socket from ClientSocketFactory.
TEST_F(DnsTransactionTest, ConcurrentLookup) {
  AddAsyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                           kT0ResponseDatagram, arraysize(kT0ResponseDatagram));
  AddAsyncQueryAndResponse(1 /* id */, kT1HostName, kT1Qtype,
                           kT1ResponseDatagram, arraysize(kT1ResponseDatagram));

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  helper0.StartTransaction(transaction_factory_.get());
  TransactionHelper helper1(kT1HostName, kT1Qtype, kT1RecordCount);
  helper1.StartTransaction(transaction_factory_.get());

  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(helper0.has_completed());
  EXPECT_TRUE(helper1.has_completed());
}

TEST_F(DnsTransactionTest, CancelLookup) {
  AddAsyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                           kT0ResponseDatagram, arraysize(kT0ResponseDatagram));
  AddAsyncQueryAndResponse(1 /* id */, kT1HostName, kT1Qtype,
                           kT1ResponseDatagram, arraysize(kT1ResponseDatagram));

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  helper0.StartTransaction(transaction_factory_.get());
  TransactionHelper helper1(kT1HostName, kT1Qtype, kT1RecordCount);
  helper1.StartTransaction(transaction_factory_.get());

  helper0.Cancel();

  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(helper0.has_completed());
  EXPECT_TRUE(helper1.has_completed());
}

TEST_F(DnsTransactionTest, DestroyFactory) {
  AddAsyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                           kT0ResponseDatagram, arraysize(kT0ResponseDatagram));

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  helper0.StartTransaction(transaction_factory_.get());

  // Destroying the client does not affect running requests.
  transaction_factory_.reset(NULL);

  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(helper0.has_completed());
}

TEST_F(DnsTransactionTest, CancelFromCallback) {
  AddAsyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                           kT0ResponseDatagram, arraysize(kT0ResponseDatagram));

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  helper0.set_cancel_in_callback();
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, MismatchedResponseSync) {
  config_.attempts = 2;
  ConfigureFactory();

  // Attempt receives mismatched response followed by valid response.
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, SYNCHRONOUS, UDP));
  data->AddResponseData(kT1ResponseDatagram,
                        arraysize(kT1ResponseDatagram), SYNCHRONOUS);
  data->AddResponseData(kT0ResponseDatagram,
                        arraysize(kT0ResponseDatagram), SYNCHRONOUS);
  AddSocketData(std::move(data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, MismatchedResponseAsync) {
  config_.attempts = 2;
  ConfigureFactory();

  // First attempt receives mismatched response followed by valid response.
  // Second attempt times out.
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, ASYNC, UDP));
  data->AddResponseData(kT1ResponseDatagram,
                        arraysize(kT1ResponseDatagram), ASYNC);
  data->AddResponseData(kT0ResponseDatagram,
                        arraysize(kT0ResponseDatagram), ASYNC);
  AddSocketData(std::move(data));
  AddQueryAndTimeout(kT0HostName, kT0Qtype);

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, MismatchedResponseFail) {
  ConfigureFactory();

  // Attempt receives mismatched response but times out because only one attempt
  // is allowed.
  AddAsyncQueryAndResponse(1 /* id */, kT0HostName, kT0Qtype,
                           kT0ResponseDatagram, arraysize(kT0ResponseDatagram));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_TIMED_OUT);
  EXPECT_FALSE(helper0.Run(transaction_factory_.get()));
  EXPECT_TRUE(helper0.FastForwardByTimeout(session_.get(), 0, 0));
}

TEST_F(DnsTransactionTest, MismatchedResponseNxdomain) {
  config_.attempts = 2;
  ConfigureFactory();

  // First attempt receives mismatched response followed by valid NXDOMAIN
  // response.
  // Second attempt receives valid NXDOMAIN response.
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, SYNCHRONOUS, UDP));
  data->AddResponseData(kT1ResponseDatagram, arraysize(kT1ResponseDatagram),
                        SYNCHRONOUS);
  data->AddRcode(dns_protocol::kRcodeNXDOMAIN, ASYNC);
  AddSocketData(std::move(data));
  AddSyncQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeNXDOMAIN);

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_NAME_NOT_RESOLVED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, ServerFail) {
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeSERVFAIL);

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_SERVER_FAILED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, NoDomain) {
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeNXDOMAIN);

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_NAME_NOT_RESOLVED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, Timeout) {
  config_.attempts = 3;
  ConfigureFactory();

  AddQueryAndTimeout(kT0HostName, kT0Qtype);
  AddQueryAndTimeout(kT0HostName, kT0Qtype);
  AddQueryAndTimeout(kT0HostName, kT0Qtype);

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_TIMED_OUT);

  // Finish when the third attempt times out.
  EXPECT_FALSE(helper0.Run(transaction_factory_.get()));
  EXPECT_FALSE(helper0.FastForwardByTimeout(session_.get(), 0, 0));
  EXPECT_FALSE(helper0.FastForwardByTimeout(session_.get(), 0, 1));
  EXPECT_TRUE(helper0.FastForwardByTimeout(session_.get(), 0, 2));
}

TEST_F(DnsTransactionTest, ServerFallbackAndRotate) {
  // Test that we fallback on both server failure and timeout.
  config_.attempts = 2;
  // The next request should start from the next server.
  config_.rotate = true;
  ConfigureNumServers(3);
  ConfigureFactory();

  // Responses for first request.
  AddQueryAndTimeout(kT0HostName, kT0Qtype);
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeSERVFAIL);
  AddQueryAndTimeout(kT0HostName, kT0Qtype);
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeSERVFAIL);
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeNXDOMAIN);
  // Responses for second request.
  AddAsyncQueryAndRcode(kT1HostName, kT1Qtype, dns_protocol::kRcodeSERVFAIL);
  AddAsyncQueryAndRcode(kT1HostName, kT1Qtype, dns_protocol::kRcodeSERVFAIL);
  AddAsyncQueryAndRcode(kT1HostName, kT1Qtype, dns_protocol::kRcodeNXDOMAIN);

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_NAME_NOT_RESOLVED);
  TransactionHelper helper1(kT1HostName, kT1Qtype, ERR_NAME_NOT_RESOLVED);

  EXPECT_FALSE(helper0.Run(transaction_factory_.get()));
  EXPECT_TRUE(helper0.FastForwardAll());
  EXPECT_TRUE(helper1.Run(transaction_factory_.get()));

  unsigned kOrder[] = {
      0, 1, 2, 0, 1,    // The first transaction.
      1, 2, 0,          // The second transaction starts from the next server.
  };
  CheckServerOrder(kOrder, arraysize(kOrder));
}

TEST_F(DnsTransactionTest, SuffixSearchAboveNdots) {
  config_.ndots = 2;
  config_.search.push_back("a");
  config_.search.push_back("b");
  config_.search.push_back("c");
  config_.rotate = true;
  ConfigureNumServers(2);
  ConfigureFactory();

  AddAsyncQueryAndRcode("x.y.z", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.y.z.a", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.y.z.b", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.y.z.c", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);

  TransactionHelper helper0("x.y.z", dns_protocol::kTypeA,
                            ERR_NAME_NOT_RESOLVED);

  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));

  // Also check if suffix search causes server rotation.
  unsigned kOrder0[] = { 0, 1, 0, 1 };
  CheckServerOrder(kOrder0, arraysize(kOrder0));
}

TEST_F(DnsTransactionTest, SuffixSearchBelowNdots) {
  config_.ndots = 2;
  config_.search.push_back("a");
  config_.search.push_back("b");
  config_.search.push_back("c");
  ConfigureFactory();

  // Responses for first transaction.
  AddAsyncQueryAndRcode("x.y.a", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.y.b", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.y.c", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.y", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  // Responses for second transaction.
  AddAsyncQueryAndRcode("x.a", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.b", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.c", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  // Responses for third transaction.
  AddAsyncQueryAndRcode("x", dns_protocol::kTypeAAAA,
                        dns_protocol::kRcodeNXDOMAIN);

  TransactionHelper helper0("x.y", dns_protocol::kTypeA, ERR_NAME_NOT_RESOLVED);

  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));

  // A single-label name.
  TransactionHelper helper1("x", dns_protocol::kTypeA, ERR_NAME_NOT_RESOLVED);

  EXPECT_TRUE(helper1.Run(transaction_factory_.get()));

  // A fully-qualified name.
  TransactionHelper helper2("x.", dns_protocol::kTypeAAAA,
                            ERR_NAME_NOT_RESOLVED);

  EXPECT_TRUE(helper2.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, EmptySuffixSearch) {
  // Responses for first transaction.
  AddAsyncQueryAndRcode("x", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);

  // A fully-qualified name.
  TransactionHelper helper0("x.", dns_protocol::kTypeA, ERR_NAME_NOT_RESOLVED);

  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));

  // A single label name is not even attempted.
  TransactionHelper helper1("singlelabel", dns_protocol::kTypeA,
                            ERR_DNS_SEARCH_EMPTY);

  helper1.Run(transaction_factory_.get());
  EXPECT_TRUE(helper1.has_completed());
}

TEST_F(DnsTransactionTest, DontAppendToMultiLabelName) {
  config_.search.push_back("a");
  config_.search.push_back("b");
  config_.search.push_back("c");
  config_.append_to_multi_label_name = false;
  ConfigureFactory();

  // Responses for first transaction.
  AddAsyncQueryAndRcode("x.y.z", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  // Responses for second transaction.
  AddAsyncQueryAndRcode("x.y", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  // Responses for third transaction.
  AddAsyncQueryAndRcode("x.a", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.b", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.c", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);

  TransactionHelper helper0("x.y.z", dns_protocol::kTypeA,
                            ERR_NAME_NOT_RESOLVED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));

  TransactionHelper helper1("x.y", dns_protocol::kTypeA, ERR_NAME_NOT_RESOLVED);
  EXPECT_TRUE(helper1.Run(transaction_factory_.get()));

  TransactionHelper helper2("x", dns_protocol::kTypeA, ERR_NAME_NOT_RESOLVED);
  EXPECT_TRUE(helper2.Run(transaction_factory_.get()));
}

const uint8_t kResponseNoData[] = {
    0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    // Question
    0x01, 'x', 0x01, 'y', 0x01, 'z', 0x01, 'b', 0x00, 0x00, 0x01, 0x00, 0x01,
    // Authority section, SOA record, TTL 0x3E6
    0x01, 'z', 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x03, 0xE6,
    // Minimal RDATA, 18 bytes
    0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

TEST_F(DnsTransactionTest, SuffixSearchStop) {
  config_.ndots = 2;
  config_.search.push_back("a");
  config_.search.push_back("b");
  config_.search.push_back("c");
  ConfigureFactory();

  AddAsyncQueryAndRcode("x.y.z", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndRcode("x.y.z.a", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddAsyncQueryAndResponse(0 /* id */, "x.y.z.b", dns_protocol::kTypeA,
                           kResponseNoData, arraysize(kResponseNoData));

  TransactionHelper helper0("x.y.z", dns_protocol::kTypeA, 0 /* answers */);

  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, SyncFirstQuery) {
  config_.search.push_back("lab.ccs.neu.edu");
  config_.search.push_back("ccs.neu.edu");
  ConfigureFactory();

  AddSyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                          kT0ResponseDatagram, arraysize(kT0ResponseDatagram));

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, SyncFirstQueryWithSearch) {
  config_.search.push_back("lab.ccs.neu.edu");
  config_.search.push_back("ccs.neu.edu");
  ConfigureFactory();

  AddSyncQueryAndRcode("www.lab.ccs.neu.edu", kT2Qtype,
                       dns_protocol::kRcodeNXDOMAIN);
  // "www.ccs.neu.edu"
  AddAsyncQueryAndResponse(2 /* id */, kT2HostName, kT2Qtype,
                           kT2ResponseDatagram, arraysize(kT2ResponseDatagram));

  TransactionHelper helper0("www", kT2Qtype, kT2RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, SyncSearchQuery) {
  config_.search.push_back("lab.ccs.neu.edu");
  config_.search.push_back("ccs.neu.edu");
  ConfigureFactory();

  AddAsyncQueryAndRcode("www.lab.ccs.neu.edu", dns_protocol::kTypeA,
                        dns_protocol::kRcodeNXDOMAIN);
  AddSyncQueryAndResponse(2 /* id */, kT2HostName, kT2Qtype,
                          kT2ResponseDatagram, arraysize(kT2ResponseDatagram));

  TransactionHelper helper0("www", kT2Qtype, kT2RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, ConnectFailure) {
  socket_factory_->fail_next_socket_ = true;
  transaction_ids_.push_back(0);  // Needed to make a DnsUDPAttempt.
  TransactionHelper helper0("www.chromium.org", dns_protocol::kTypeA,
                            ERR_CONNECTION_REFUSED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, ConnectFailureFollowedBySuccess) {
  // Retry after server failure.
  config_.attempts = 2;
  ConfigureFactory();
  // First server connection attempt fails.
  transaction_ids_.push_back(0);  // Needed to make a DnsUDPAttempt.
  socket_factory_->fail_next_socket_ = true;
  // Second DNS query succeeds.
  AddAsyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                           kT0ResponseDatagram, arraysize(kT0ResponseDatagram));
  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, HTTPSGetLookup) {
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, HTTPS);
  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  ConfigDoHServers(true, false, &helper0);
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
  ShutdownDohServers();
}

TEST_F(DnsTransactionTest, HTTPSGetFailure) {
  AddQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeSERVFAIL, ASYNC,
                   HTTPS);

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_SERVER_FAILED);
  ConfigDoHServers(true, false, &helper0);
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, HTTPSGetMalformed) {
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0, kT0HostName, kT0Qtype, ASYNC, HTTPS));
  std::unique_ptr<DnsResponse> response = std::make_unique<DnsResponse>(
      reinterpret_cast<const char*>(kT0ResponseDatagram),
      arraysize(kT0ResponseDatagram), 0);
  // Change the id of the header to make the response malformed.
  response->io_buffer()->data()[0]++;
  data->AddResponse(std::move(response), ASYNC);
  AddSocketData(std::move(data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_MALFORMED_RESPONSE);
  ConfigDoHServers(true, false, &helper0);
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, HTTPSPostLookup) {
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, HTTPS);
  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  ConfigDoHServers(true, true, &helper0);
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, HTTPSPostFailure) {
  AddQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeSERVFAIL, ASYNC,
                   HTTPS);

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_SERVER_FAILED);
  ConfigDoHServers(true, true, &helper0);
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, HTTPSPostMalformed) {
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0, kT0HostName, kT0Qtype, ASYNC, HTTPS));
  std::unique_ptr<DnsResponse> response = std::make_unique<DnsResponse>(
      reinterpret_cast<const char*>(kT0ResponseDatagram),
      arraysize(kT0ResponseDatagram), 0);
  // Change the id of the header to make the response malformed.
  response->io_buffer()->data()[0]++;
  data->AddResponse(std::move(response), ASYNC);
  AddSocketData(std::move(data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_MALFORMED_RESPONSE);
  ConfigDoHServers(true, true, &helper0);
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

class AsyncHttpResponse : public test_server::BasicHttpResponse {
 public:
  AsyncHttpResponse(std::string content) : content_(content) {
    set_content(content);
    set_content_type("application/dns-udpwireformat");
  }

  void SendResponse(const test_server::SendBytesCallback& send,
                    const test_server::SendCompleteCallback& done) override {
    std::string response = ToResponseString();
    send.Run(response.substr(0, response.size() - content_.size()),
             base::Bind(&base::DoNothing));
    std::string rest = response.substr(response.size() - content_.size());
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&AsyncHttpResponse::FinishSend, base::Unretained(this), send,
                   done, rest),
        base::TimeDelta::FromMilliseconds(200));
  }

  void FinishSend(const test_server::SendBytesCallback& send,
                  const test_server::SendCompleteCallback& done,
                  std::string rest_of_response) {
    send.Run(rest_of_response, done);
  }

 private:
  std::string content_;
  DISALLOW_COPY_AND_ASSIGN(AsyncHttpResponse);
};

std::unique_ptr<test_server::HttpResponse> MakeAsyncResponse(
    const net::test_server::HttpRequest&,
    std::string content) {
  return std::make_unique<AsyncHttpResponse>(content);
}

TEST_F(DnsTransactionTest, HTTPSPostLookupAsync) {
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, HTTPS);
  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  ConfigDoHServers(true, true, &helper0,
                   base::BindRepeating(MakeAsyncResponse));
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

std::unique_ptr<test_server::HttpResponse> MakeShortResponse(
    const net::test_server::HttpRequest&,
    std::string content) {
  std::unique_ptr<test_server::RawHttpResponse> resp =
      std::make_unique<test_server::RawHttpResponse>("", content);
  resp->AddHeader("HTTP/1.1 200 OK");
  resp->AddHeader("Connection: close");
  resp->AddHeader("Content-Type: application/dns-udpwireformat");
  resp->AddHeader("Content-Length: 3");
  return resp;
}

TEST_F(DnsTransactionTest, HTTPSPostContentLengthShort) {
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, HTTPS);
  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_MALFORMED_RESPONSE);
  ConfigDoHServers(true, true, &helper0,
                   base::BindRepeating(MakeShortResponse));
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

std::unique_ptr<test_server::HttpResponse> MakeLongResponse(
    const net::test_server::HttpRequest&,
    std::string content) {
  std::unique_ptr<test_server::RawHttpResponse> resp =
      std::make_unique<test_server::RawHttpResponse>("", content);
  resp->AddHeader("HTTP/1.1 200 OK");
  resp->AddHeader("Connection: close");
  resp->AddHeader("Content-Type: application/dns-udpwireformat");
  resp->AddHeader("Content-Length: 300");
  return resp;
}

TEST_F(DnsTransactionTest, HTTPSPostContentLengthLong) {
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, HTTPS);
  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_CONTENT_LENGTH_MISMATCH);
  ConfigDoHServers(true, true, &helper0, base::BindRepeating(MakeLongResponse));
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

std::unique_ptr<test_server::HttpResponse> MakeResponseWithoutLength(
    const net::test_server::HttpRequest&,
    std::string content) {
  std::unique_ptr<test_server::RawHttpResponse> resp =
      std::make_unique<test_server::RawHttpResponse>("", content);
  resp->AddHeader("HTTP/1.1 200 OK");
  resp->AddHeader("Connection: close");
  resp->AddHeader("Content-Type: application/dns-udpwireformat");
  return resp;
}

TEST_F(DnsTransactionTest, HTTPSPostNoContentLength) {
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, HTTPS);
  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  ConfigDoHServers(true, true, &helper0,
                   base::BindRepeating(MakeResponseWithoutLength));
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

std::unique_ptr<test_server::HttpResponse> MakeResponseWithError(
    const net::test_server::HttpRequest& req,
    std::string content) {
  std::unique_ptr<test_server::BasicHttpResponse> resp(
      static_cast<test_server::BasicHttpResponse*>(
          DnsTransactionTest::MakeResponse(req, content).release()));
  resp->set_code(HTTP_INTERNAL_SERVER_ERROR);
  return resp;
}

TEST_F(DnsTransactionTest, HTTPSPostWithError) {
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, HTTPS);
  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_MALFORMED_RESPONSE);
  ConfigDoHServers(true, true, &helper0,
                   base::BindRepeating(MakeResponseWithError));
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

std::unique_ptr<test_server::HttpResponse> MakeResponseWrongType(
    const net::test_server::HttpRequest& req,
    std::string content) {
  std::unique_ptr<test_server::BasicHttpResponse> resp(
      static_cast<test_server::BasicHttpResponse*>(
          DnsTransactionTest::MakeResponse(req, content).release()));
  resp->set_content_type("test/html");
  return resp;
}

TEST_F(DnsTransactionTest, HTTPSPostWithWrongType) {
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, HTTPS);
  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_MALFORMED_RESPONSE);
  ConfigDoHServers(true, true, &helper0,
                   base::BindRepeating(MakeResponseWrongType));
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

std::unique_ptr<test_server::HttpResponse> MakeResponseNoType(
    const net::test_server::HttpRequest&,
    std::string content) {
  std::unique_ptr<test_server::RawHttpResponse> resp =
      std::make_unique<test_server::RawHttpResponse>("", content);
  resp->AddHeader("HTTP/1.1 200 OK");
  resp->AddHeader("Connection: close");
  resp->AddHeader(base::StringPrintf(
      "Content-Length: %lu", static_cast<unsigned long>(content.length())));
  return resp;
}

TEST_F(DnsTransactionTest, HTTPSPostWithNoType) {
  AddQueryAndResponse(0, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, HTTPS);
  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_MALFORMED_RESPONSE);
  ConfigDoHServers(true, true, &helper0,
                   base::BindRepeating(MakeResponseNoType));
  EXPECT_TRUE(helper0.RunUntilDone(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, TCPLookup) {
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype,
                        dns_protocol::kRcodeNOERROR | dns_protocol::kFlagTC);
  AddQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype, kT0ResponseDatagram,
                      arraysize(kT0ResponseDatagram), ASYNC, TCP);

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, TCPFailure) {
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype,
                        dns_protocol::kRcodeNOERROR | dns_protocol::kFlagTC);
  AddQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeSERVFAIL, ASYNC,
                   TCP);

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_SERVER_FAILED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, TCPMalformed) {
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype,
                        dns_protocol::kRcodeNOERROR | dns_protocol::kFlagTC);
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, ASYNC, TCP));
  // Valid response but length too short.
  // This must be truncated in the question section. The DnsResponse doesn't
  // examine the answer section until asked to parse it, so truncating it in
  // the answer section would result in the DnsTransaction itself succeeding.
  data->AddResponseWithLength(
      std::make_unique<DnsResponse>(
          reinterpret_cast<const char*>(kT0ResponseDatagram),
          arraysize(kT0ResponseDatagram), 0),
      ASYNC, static_cast<uint16_t>(kT0QuerySize - 1));
  AddSocketData(std::move(data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_MALFORMED_RESPONSE);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, TCPTimeout) {
  ConfigureFactory();
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype,
                        dns_protocol::kRcodeNOERROR | dns_protocol::kFlagTC);
  AddSocketData(std::make_unique<DnsSocketData>(1 /* id */, kT0HostName,
                                                kT0Qtype, ASYNC, TCP));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_DNS_TIMED_OUT);
  EXPECT_FALSE(helper0.Run(transaction_factory_.get()));
  EXPECT_TRUE(helper0.FastForwardAll());
}

TEST_F(DnsTransactionTest, TCPReadReturnsZeroAsync) {
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype,
                        dns_protocol::kRcodeNOERROR | dns_protocol::kFlagTC);
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, ASYNC, TCP));
  // Return all but the last byte of the response.
  data->AddResponseWithLength(
      std::make_unique<DnsResponse>(
          reinterpret_cast<const char*>(kT0ResponseDatagram),
          arraysize(kT0ResponseDatagram) - 1, 0),
      ASYNC, static_cast<uint16_t>(arraysize(kT0ResponseDatagram)));
  // Then return a 0-length read.
  data->AddReadError(0, ASYNC);
  AddSocketData(std::move(data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_CONNECTION_CLOSED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, TCPReadReturnsZeroSynchronous) {
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype,
                        dns_protocol::kRcodeNOERROR | dns_protocol::kFlagTC);
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, ASYNC, TCP));
  // Return all but the last byte of the response.
  data->AddResponseWithLength(
      std::make_unique<DnsResponse>(
          reinterpret_cast<const char*>(kT0ResponseDatagram),
          arraysize(kT0ResponseDatagram) - 1, 0),
      SYNCHRONOUS, static_cast<uint16_t>(arraysize(kT0ResponseDatagram)));
  // Then return a 0-length read.
  data->AddReadError(0, SYNCHRONOUS);
  AddSocketData(std::move(data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_CONNECTION_CLOSED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, TCPConnectionClosedAsync) {
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype,
                        dns_protocol::kRcodeNOERROR | dns_protocol::kFlagTC);
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, ASYNC, TCP));
  data->AddReadError(ERR_CONNECTION_CLOSED, ASYNC);
  AddSocketData(std::move(data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_CONNECTION_CLOSED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, TCPConnectionClosedSynchronous) {
  AddAsyncQueryAndRcode(kT0HostName, kT0Qtype,
                        dns_protocol::kRcodeNOERROR | dns_protocol::kFlagTC);
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, ASYNC, TCP));
  data->AddReadError(ERR_CONNECTION_CLOSED, SYNCHRONOUS);
  AddSocketData(std::move(data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_CONNECTION_CLOSED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, MismatchedThenNxdomainThenTCP) {
  config_.attempts = 2;
  ConfigureFactory();
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, SYNCHRONOUS, UDP));
  // First attempt gets a mismatched response.
  data->AddResponseData(kT1ResponseDatagram, arraysize(kT1ResponseDatagram),
                        SYNCHRONOUS);
  // Second read from first attempt gets TCP required.
  data->AddRcode(dns_protocol::kFlagTC, ASYNC);
  AddSocketData(std::move(data));
  // Second attempt gets NXDOMAIN, which happens before the TCP required.
  AddSyncQueryAndRcode(kT0HostName, kT0Qtype, dns_protocol::kRcodeNXDOMAIN);
  std::unique_ptr<DnsSocketData> tcp_data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, ASYNC, TCP));
  tcp_data->AddReadError(ERR_CONNECTION_CLOSED, SYNCHRONOUS);
  AddSocketData(std::move(tcp_data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, ERR_NAME_NOT_RESOLVED);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, MismatchedThenOkThenTCP) {
  config_.attempts = 2;
  ConfigureFactory();
  std::unique_ptr<DnsSocketData> data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, SYNCHRONOUS, UDP));
  // First attempt gets a mismatched response.
  data->AddResponseData(kT1ResponseDatagram, arraysize(kT1ResponseDatagram),
                        SYNCHRONOUS);
  // Second read from first attempt gets TCP required.
  data->AddRcode(dns_protocol::kFlagTC, ASYNC);
  AddSocketData(std::move(data));
  // Second attempt gets a valid response, which happens before the TCP
  // required.
  AddSyncQueryAndResponse(0 /* id */, kT0HostName, kT0Qtype,
                          kT0ResponseDatagram, arraysize(kT0ResponseDatagram));
  std::unique_ptr<DnsSocketData> tcp_data(
      new DnsSocketData(0 /* id */, kT0HostName, kT0Qtype, ASYNC, TCP));
  tcp_data->AddReadError(ERR_CONNECTION_CLOSED, SYNCHRONOUS);
  AddSocketData(std::move(tcp_data));

  TransactionHelper helper0(kT0HostName, kT0Qtype, kT0RecordCount);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));
}

TEST_F(DnsTransactionTest, InvalidQuery) {
  ConfigureFactory();

  TransactionHelper helper0(".", dns_protocol::kTypeA, ERR_INVALID_ARGUMENT);
  EXPECT_TRUE(helper0.Run(transaction_factory_.get()));

  TransactionHelper helper1("foo,bar.com", dns_protocol::kTypeA,
                            ERR_INVALID_ARGUMENT);
  EXPECT_TRUE(helper1.Run(transaction_factory_.get()));
}

}  // namespace

}  // namespace net
