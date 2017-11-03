// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/chromium/quic_stream_factory.h"

#include "base/test/fuzzed_data_provider.h"

#include "net/base/test_completion_callback.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/dns/fuzzed_host_resolver.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/transport_security_state.h"
#include "net/quic/chromium/mock_crypto_client_stream_factory.h"
#include "net/quic/chromium/test_task_runner.h"
#include "net/quic/test_tools/mock_clock.h"
#include "net/quic/test_tools/mock_random.h"
#include "net/socket/fuzzed_datagram_client_socket.h"
#include "net/socket/fuzzed_socket_factory.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"

namespace net {

namespace {

class MockSSLConfigService : public SSLConfigService {
 public:
  MockSSLConfigService() {}

  void GetSSLConfig(SSLConfig* config) override { *config = config_; }

 private:
  ~MockSSLConfigService() override {}

  SSLConfig config_;
};

}  // namespace

namespace test {

const char kDefaultServerHostName[] = "www.example.org";
const int kDefaultServerPort = 443;
const char kDefaultUrl[] = "https://www.example.org/";
// TODO(nedwilliamson): Add POST here after testing
// whether that can lead blocking while waiting for
// the callbacks.
const char kMethod[] = "GET";
const size_t kBufferSize = 4096;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  base::FuzzedDataProvider fdp(data, size);

  FuzzedHostResolver host_resolver(HostResolver::Options(), nullptr, &fdp);
  scoped_refptr<SSLConfigService> ssl_config_service_ =
      new MockSSLConfigService;
  FuzzedSocketFactory socket_factory(&fdp);
  MockCryptoClientStreamFactory crypto_client_stream_factory;
  crypto_client_stream_factory.set_use_mock_crypter(true);
  MockRandom random_generator(0);
  MockClock clock;
  scoped_refptr<TestTaskRunner> runner_ = new TestTaskRunner(&clock);
  HttpServerPropertiesImpl http_server_properties;
  std::unique_ptr<CertVerifier> cert_verifier_ = CertVerifier::CreateDefault();
  std::unique_ptr<ChannelIDService> channel_id_service_ =
      std::make_unique<ChannelIDService>(new DefaultChannelIDStore(nullptr));
  TransportSecurityState transport_security_state;
  std::unique_ptr<CTVerifier> cert_transparency_verifier_ =
      std::make_unique<MultiLogCTVerifier>();
  CTPolicyEnforcer ct_policy_enforcer;
  QuicTagVector connection_options;
  QuicTagVector client_connection_options;

  clock.AdvanceTime(QuicTime::Delta::FromSeconds(1));

  NetLogWithSource net_log;

  std::unique_ptr<QuicStreamFactory> factory =
      std::make_unique<QuicStreamFactory>(
          net_log.net_log(), &host_resolver, ssl_config_service_.get(),
          &socket_factory, &http_server_properties, cert_verifier_.get(),
          &ct_policy_enforcer, channel_id_service_.get(),
          &transport_security_state, cert_transparency_verifier_.get(), nullptr,
          &crypto_client_stream_factory, &random_generator, &clock,
          kDefaultMaxPacketSize, std::string(), false, false,
          kIdleConnectionTimeoutSeconds, kPingTimeoutSecs, true, false, false,
          false, false, false, connection_options, client_connection_options,
          false);

  HostPortPair host_port_pair(kDefaultServerHostName, kDefaultServerPort);
  QuicStreamRequest request(factory.get());
  TestCompletionCallback callback;
  NetErrorDetails net_error_details;
  request.Request(host_port_pair,
                  fdp.PickValueInArray(kSupportedTransportVersions),
                  PRIVACY_MODE_DISABLED, 0, GURL(kDefaultUrl), kMethod, net_log,
                  &net_error_details, callback.callback());

  callback.WaitForResult();
  std::unique_ptr<HttpStream> stream = request.CreateStream();
  if (!stream.get()) {
    return 0;
  }

  HttpRequestInfo request_info;
  request_info.method = kMethod;
  request_info.url = GURL(kDefaultUrl);
  stream->InitializeStream(&request_info, DEFAULT_PRIORITY, net_log,
                           CompletionCallback());

  HttpResponseInfo response;
  HttpRequestHeaders request_headers;
  if (OK !=
      stream->SendRequest(request_headers, &response, callback.callback())) {
    return 0;
  }

  // TODO(nedwilliamson): attempt connection migration here
  stream->ReadResponseHeaders(callback.callback());
  callback.WaitForResult();

  scoped_refptr<net::IOBuffer> buffer = new net::IOBuffer(kBufferSize);
  int rv =
      stream->ReadResponseBody(buffer.get(), kBufferSize, callback.callback());
  if (rv == ERR_IO_PENDING)
    callback.WaitForResult();

  return 0;
}

}  // namespace test
}  // namespace net
