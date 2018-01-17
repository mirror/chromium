// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/grpc_support/test/quic_test_server.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/chromium/crypto/proof_source_chromium.h"
#include "net/spdy/core/spdy_header_block.h"
#include "net/test/test_data_directory.h"
#include "net/tools/quic/quic_http_response_cache.h"
#include "net/tools/quic/quic_simple_server.h"

namespace grpc_support {

const char kTestServerDomain[] = "example.com";
// This must match the certificate used (quic-chain.pem and quic-leaf-cert.key).
const char kTestServerHost[] = "test.example.com";
const char kTestServerUrl[] = "https://test.example.com/hello.txt";

const char kStatusHeader[] = ":status";

const char kHelloPath[] = "/hello.txt";
const char kHelloBodyValue[] = "Hello from QUIC Server";
const char kHelloStatus[] = "200";

const char kHelloHeaderName[] = "hello_header";
const char kHelloHeaderValue[] = "hello header value";

const char kHelloTrailerName[] = "hello_trailer";
const char kHelloTrailerValue[] = "hello trailer value";

const char kTestServerSimpleUrl[] = "https://test.example.com/simple.txt";
const char kSimplePath[] = "/simple.txt";
const char kSimpleBodyValue[] = "Simple Hello from QUIC Server";
const char kSimpleStatus[] = "200";

const char kSimpleHeaderName[] = "hello_header";
const char kSimpleHeaderValue[] = "hello header value";

std::unique_ptr<base::Thread> g_quic_server_thread;
std::unique_ptr<net::QuicHttpResponseCache> g_quic_response_cache;
std::unique_ptr<net::QuicSimpleServer> g_quic_server;
int g_quic_server_port = 0;

void SetupQuicHttpResponseCache() {
  net::SpdyHeaderBlock headers;
  headers[kHelloHeaderName] = kHelloHeaderValue;
  headers[kStatusHeader] =  kHelloStatus;
  net::SpdyHeaderBlock trailers;
  trailers[kHelloTrailerName] = kHelloTrailerValue;
  g_quic_response_cache = std::make_unique<net::QuicHttpResponseCache>();
  g_quic_response_cache->AddResponse(base::StringPrintf("%s", kTestServerHost),
                                      kHelloPath, std::move(headers),
                                      kHelloBodyValue, std::move(trailers));
  headers[kSimpleHeaderName] = kSimpleHeaderValue;
  headers[kStatusHeader] = kSimpleStatus;
  g_quic_response_cache->AddResponse(base::StringPrintf("%s", kTestServerHost),
                                     kSimplePath, std::move(headers),
                                     kSimpleBodyValue);
}

void StartQuicServerOnServerThread(const base::FilePath& test_files_root,
                                   base::WaitableEvent* server_started_event) {
  DCHECK(g_quic_server_thread->task_runner()->BelongsToCurrentThread());
  DCHECK(!g_quic_server);

  net::QuicConfig config;
  // Set up server certs.
  base::FilePath directory;
  directory = test_files_root;
  std::unique_ptr<net::ProofSourceChromium> proof_source =
      std::make_unique<net::ProofSourceChromium>();
  CHECK(proof_source->Initialize(directory.AppendASCII("quic-chain.pem"),
                                 directory.AppendASCII("quic-leaf-cert.key"),
                                 base::FilePath()));
  SetupQuicHttpResponseCache();

  g_quic_server = std::make_unique<net::QuicSimpleServer>(
      std::move(proof_source), config,
      net::QuicCryptoServerConfig::ConfigOptions(), net::AllSupportedVersions(),
      g_quic_response_cache.get());

  // Start listening on an unbound port.
  int rv = g_quic_server->Listen(
      net::IPEndPoint(net::IPAddress::IPv4AllZeros(), 0));
  CHECK_GE(rv, 0) << "Quic server fails to start";
  g_quic_server_port = g_quic_server->server_address().port();
  server_started_event->Signal();
}

void ShutdownOnServerThread(base::WaitableEvent* server_stopped_event) {
  DCHECK(g_quic_server_thread->task_runner()->BelongsToCurrentThread());
  g_quic_server->Shutdown();
  g_quic_server.reset();
  g_quic_response_cache.reset();
  server_stopped_event->Signal();
}

bool StartQuicTestServer() {
  LOG(INFO) << g_quic_server_thread.get();
  DCHECK(!g_quic_server_thread);
  g_quic_server_thread = std::make_unique<base::Thread>("quic server thread");
  base::Thread::Options thread_options;
  thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
  bool started = g_quic_server_thread->StartWithOptions(thread_options);
  DCHECK(started);
  base::FilePath test_files_root = net::GetTestCertsDirectory();

  base::WaitableEvent server_started_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  g_quic_server_thread->task_runner()->PostTask(
      FROM_HERE, base::Bind(&StartQuicServerOnServerThread, test_files_root,
                            &server_started_event));
  server_started_event.Wait();
  return true;
}

void DeleteQuicServerThread(std::unique_ptr<base::Thread> quic_server_thread) {
  // |g_quic_server_thread| is moved here to be deleted on MayBlock thread.
  DCHECK(!g_quic_server_thread.get());
  DCHECK(quic_server_thread.get());
  quic_server_thread.reset();
}

void ShutdownQuicTestServer() {
  if (!g_quic_server_thread)
    return;
  DCHECK(!g_quic_server_thread->task_runner()->BelongsToCurrentThread());
  base::WaitableEvent server_stopped_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  g_quic_server_thread->task_runner()->PostTask(
      FROM_HERE, base::Bind(&ShutdownOnServerThread, &server_stopped_event));
  server_stopped_event.Wait();
  // ShutdownQuicTestServer() may be called on network thread which doesn't
  // allow blocking, so post the deletion to task scheduler.
  /*
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&DeleteQuicServerThread, std::move(g_quic_server_thread)));
      */
  g_quic_server_thread.reset();
}

int GetQuicTestServerPort() {
  return g_quic_server_port;
}

}  // namespace grpc_support
