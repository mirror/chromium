// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/quic_simple_test_server.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/chromium/crypto/proof_source_chromium.h"
#include "net/spdy/core/spdy_header_block.h"
#include "net/test/test_data_directory.h"
#include "net/tools/quic/quic_dispatcher.h"
#include "net/tools/quic/quic_http_response_cache.h"
#include "net/tools/quic/quic_simple_server.h"

namespace net {

const char QuicSimpleTestServer::kTestServerDomain[] = "example.com";
// This must match the certificate used (quic-chain.pem and quic-leaf-cert.key).
const char QuicSimpleTestServer::kTestServerHost[] = "test.example.com";
const char QuicSimpleTestServer::kTestServerUrl[] =
    "https://test.example.com/hello.txt";

const char QuicSimpleTestServer::kStatusHeader[] = ":status";

const char QuicSimpleTestServer::kHelloPath[] = "/hello.txt";
const char QuicSimpleTestServer::kHelloBodyValue[] = "Hello from QUIC Server";
const char QuicSimpleTestServer::kHelloStatus[] = "200";

const char QuicSimpleTestServer::kHelloHeaderName[] = "hello_header";
const char QuicSimpleTestServer::kHelloHeaderValue[] = "hello header value";

const char QuicSimpleTestServer::kHelloTrailerName[] = "hello_trailer";
const char QuicSimpleTestServer::kHelloTrailerValue[] = "hello trailer value";

const char QuicSimpleTestServer::kTestServerSimpleUrl[] =
    "https://test.example.com/simple.txt";
const char kSimplePath[] = "/simple.txt";
const char QuicSimpleTestServer::kSimpleBodyValue[] =
    "Simple Hello from QUIC Server";
const char QuicSimpleTestServer::kSimpleStatus[] = "200";

const char QuicSimpleTestServer::kSimpleHeaderName[] = "hello_header";
const char QuicSimpleTestServer::kSimpleHeaderValue[] = "hello header value";

base::Thread* g_quic_server_thread = nullptr;
QuicHttpResponseCache* g_quic_response_cache = nullptr;
QuicSimpleServer* g_quic_server = nullptr;
int g_quic_server_port = 0;

void SetupQuicHttpResponseCache() {
  SpdyHeaderBlock headers;
  headers[QuicSimpleTestServer::kHelloHeaderName] =
      QuicSimpleTestServer::kHelloHeaderValue;
  headers[QuicSimpleTestServer::kStatusHeader] =
      QuicSimpleTestServer::kHelloStatus;
  SpdyHeaderBlock trailers;
  trailers[QuicSimpleTestServer::kHelloTrailerName] =
      QuicSimpleTestServer::kHelloTrailerValue;
  g_quic_response_cache = new QuicHttpResponseCache();
  g_quic_response_cache->AddResponse(
      base::StringPrintf("%s", QuicSimpleTestServer::kTestServerHost),
      QuicSimpleTestServer::kHelloPath, std::move(headers),
      QuicSimpleTestServer::kHelloBodyValue, std::move(trailers));
  headers[QuicSimpleTestServer::kSimpleHeaderName] =
      QuicSimpleTestServer::kSimpleHeaderValue;
  headers[QuicSimpleTestServer::kStatusHeader] =
      QuicSimpleTestServer::kSimpleStatus;
  g_quic_response_cache->AddResponse(
      base::StringPrintf("%s", QuicSimpleTestServer::kTestServerHost),
      kSimplePath, std::move(headers), QuicSimpleTestServer::kSimpleBodyValue);
}

void StartQuicServerOnServerThread(const base::FilePath& test_files_root,
                                   base::WaitableEvent* server_started_event) {
  DCHECK(g_quic_server_thread->task_runner()->BelongsToCurrentThread());
  DCHECK(!g_quic_server);

  QuicConfig config;
  // Set up server certs.
  base::FilePath directory;
  directory = test_files_root;
  std::unique_ptr<ProofSourceChromium> proof_source(new ProofSourceChromium());
  CHECK(proof_source->Initialize(directory.AppendASCII("quic-chain.pem"),
                                 directory.AppendASCII("quic-leaf-cert.key"),
                                 base::FilePath()));
  SetupQuicHttpResponseCache();

  g_quic_server = new QuicSimpleServer(
      std::move(proof_source), config, QuicCryptoServerConfig::ConfigOptions(),
      AllSupportedVersions(), g_quic_response_cache);

  // Start listening on an unbound port.
  int rv = g_quic_server->Listen(IPEndPoint(IPAddress::IPv4AllZeros(), 0));
  CHECK_GE(rv, 0) << "Quic server fails to start";
  g_quic_server_port = g_quic_server->server_address().port();
  server_started_event->Signal();
}

void ShutdownOnServerThread(base::WaitableEvent* server_stopped_event) {
  DCHECK(g_quic_server_thread->task_runner()->BelongsToCurrentThread());
  g_quic_server->Shutdown();
  delete g_quic_server;
  g_quic_server = nullptr;
  delete g_quic_response_cache;
  g_quic_response_cache = nullptr;
  server_stopped_event->Signal();
}

void ShutdownDispatcherOnServerThread(
    base::WaitableEvent* dispatcher_stopped_event) {
  DCHECK(g_quic_server_thread->task_runner()->BelongsToCurrentThread());
  g_quic_server->dispatcher()->Shutdown();
  dispatcher_stopped_event->Signal();
}

bool QuicSimpleTestServer::Start() {
  DVLOG(3) << g_quic_server_thread;
  DCHECK(!g_quic_server_thread);
  g_quic_server_thread = new base::Thread("quic server thread");
  base::Thread::Options thread_options;
  thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
  bool started = g_quic_server_thread->StartWithOptions(thread_options);
  DCHECK(started);
  base::FilePath test_files_root = GetTestCertsDirectory();

  base::WaitableEvent server_started_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  g_quic_server_thread->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&StartQuicServerOnServerThread, test_files_root,
                                &server_started_event));
  server_started_event.Wait();
  return true;
}

// Shut down the server dispatcher, and the stream should error out.
void QuicSimpleTestServer::ShutdownDispatcher() {
  if (!g_quic_server)
    return;
  DCHECK(!g_quic_server_thread->task_runner()->BelongsToCurrentThread());
  base::WaitableEvent dispatcher_stopped_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  g_quic_server_thread->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&ShutdownDispatcherOnServerThread,
                                &dispatcher_stopped_event));
  dispatcher_stopped_event.Wait();
}

void QuicSimpleTestServer::Shutdown() {
  if (!g_quic_server)
    return;
  DCHECK(!g_quic_server_thread->task_runner()->BelongsToCurrentThread());
  base::WaitableEvent server_stopped_event(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  g_quic_server_thread->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&ShutdownOnServerThread, &server_stopped_event));
  server_stopped_event.Wait();
  delete g_quic_server_thread;
  g_quic_server_thread = nullptr;
}

int QuicSimpleTestServer::GetPort() {
  return g_quic_server_port;
}

}  // namespace net
