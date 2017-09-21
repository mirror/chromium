// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A toy server, which listens on a specified address for QUIC traffic and
// handles incoming responses.
//
// Note that this server is intended to verify correctness of the client and is
// in no way expected to be performant.
//
// The quic_server can be additionally used as a reverse proxy using the
// --mode=proxy argument.
//
// To add the functionality of a reverse proxy to the QUIC server, 2 classes
// were added:
// 1. QuicHttpResponseProxy: Creates a proxy thread and manages an instance of
//    net::URLRequestContext within that thread to make HTTP calls to a backend
//    server.
// 2. QuicProxyBackendUrlRequest: Created on a per-stream basis, manages an
//    instance of the class net::URLRequest to make a single HTTP call to the
//    backend server using the context created by QuicHttpResponseProxy.
// And interfaced with the following class:
// QuicSimpleServerStream: Create a new QuicProxyBackendUrlRequest instance for
// every QUIC stream and sends the HTTP request to the instance and implements
// the callback to receive the HTTP response. Once the entire HTTP request is
// received from the client by a specific QUIC stream (aka an object of class
// QuicSimpleServerStream), the request is then passed to an instance of the
// class QuicProxyBackendUrlRequest, that is created and destroyed by the
// QuicHttpResponseProxy instance. The QuicProxyBackendUrlRequest instance sends
// and receives the response from the backend on the quic proxy thread,
// allowing the main thread to continue processing other QUIC connections.
// QuicSimpleServerStream implements the callback to receive the response from
// QuicProxyBackendUrlRequest, which is then sent back to the QUIC client on
// the main thread.
//
// quic_proxy_backend_url_request.h has a description of threads, the flow
// of packets in QUIC proxy in the forward and reverse directions.
//
// There are two threads in the QUIC proxy, a main/quic thread and a quic proxy
// thread. In the forward direction, the quic thread handles the front end
// QUIC connections/ streams/ packets and posts the packets to the quic proxy
// thread which forwards the packets over the TCP sockets to the backend Web
// servers. In the reverse direction, the HTTP packets are received by the
// quic proxy thread which posts a task with an HTTP response packet to the
// QUIC thread. The QUIC thread serves the task and forwards the HTTP packets
// into the corresponding connection/ stream which the HTTP response is
// correlated to.
//
// The flow of the packets between a QUIC client, a QUIC proxy and a backend
// HTTP server is listed below:
// 1. Request packets coming from the QUIC client are processed in the
//    QuicSimpleServerStream by the quic thread.
// 2. QuickSimpleServerStream creates a new QuicProxyBackendUrlRequest for a
//    new QUIC stream to store the client address, connection ID
//    (QuicSimpleServerStream::OnInitialHeadersComplete). The subsequent HTTP
//    body packets are stored in the QUIC stream. Once the last packet in a
//    QUIC stream is received (QuicSimpleServerStream::SendResponse()), the
//    pointers to HTTP headers and body are passed to the task which is post
//    (QuicProxyBackendUrlRequest::SendRequestToBackend()) to the quic proxy
//    thread asynchronously.
// 3. Quic proxy thread serves the tasks (SendRequestOnBackendThread()) and
//    sends the HTTP request to the backend server over a TCP socket
//    (URLRequest::Start()).
// 4. Once the HTTP response is received by the quic proxy thread, it calls
//    the callbacks (QuicProxyBackendUrlRequest::OnResponseCompleted()) which
//    stores the HTTP response in QuicProxyBackendUrlRequest and posts a task
//    back to the quic thread.
// 5. The quic thread processes the task
//    (QuicProxyBackendUrlRequest::SendResponseOnQuicThread()) to return the
//    HTTP response back to the corresponding QUIC stream (associated with the
//    connection ID recorded in step 2).
//
// quic_proxy_backend_url_request.h has a description of threads, the flow
// of packets in QUIC proxy in the forward and reverse directions.

#ifndef NET_TOOLS_QUIC_QUIC_SERVER_H_
#define NET_TOOLS_QUIC_QUIC_SERVER_H_

#include <memory>

#include "base/macros.h"
#include "net/quic/chromium/quic_chromium_connection_helper.h"
#include "net/quic/core/crypto/quic_crypto_server_config.h"
#include "net/quic/core/quic_config.h"
#include "net/quic/core/quic_framer.h"
#include "net/quic/core/quic_version_manager.h"
#include "net/quic/platform/api/quic_socket_address.h"
#include "net/tools/epoll_server/epoll_server.h"
#include "net/tools/quic/quic_default_packet_writer.h"
#include "net/tools/quic/quic_http_response_cache.h"
#include "net/tools/quic/quic_http_response_proxy.h"

namespace net {

namespace test {
class QuicServerPeer;
}  // namespace test

class QuicDispatcher;
class QuicPacketReader;

class QuicServer : public EpollCallbackInterface {
 public:
  QuicServer(std::unique_ptr<ProofSource> proof_source,
             QuicHttpResponseCache* response_cache);

  QuicServer(std::unique_ptr<ProofSource> proof_source,
             QuicHttpResponseCache* response_cache,
             QuicHttpResponseProxy* quic_proxy_context);

  QuicServer(std::unique_ptr<ProofSource> proof_source,
             const QuicConfig& config,
             const QuicCryptoServerConfig::ConfigOptions& server_config_options,
             const QuicVersionVector& supported_versions,
             QuicHttpResponseCache* response_cache);

  QuicServer(std::unique_ptr<ProofSource> proof_source,
             const QuicConfig& config,
             const QuicCryptoServerConfig::ConfigOptions& server_config_options,
             const QuicVersionVector& supported_versions,
             QuicHttpResponseCache* response_cache,
             QuicHttpResponseProxy* quic_proxy_context);

  ~QuicServer() override;

  // Start listening on the specified address.
  bool CreateUDPSocketAndListen(const QuicSocketAddress& address);

  // Wait up to 50ms, and handle any events which occur.
  void WaitForEvents();

  void Start();
  void Run();

  // Server deletion is imminent.  Start cleaning up the epoll server.
  virtual void Shutdown();

  // From EpollCallbackInterface
  void OnRegistration(EpollServer* eps, int fd, int event_mask) override {}
  void OnModification(int fd, int event_mask) override {}
  void OnEvent(int fd, EpollEvent* event) override;
  void OnUnregistration(int fd, bool replaced) override {}

  void OnShutdown(EpollServer* eps, int fd) override {}

  void SetChloMultiplier(size_t multiplier) {
    crypto_config_.set_chlo_multiplier(multiplier);
  }

  bool overflow_supported() { return overflow_supported_; }

  QuicPacketCount packets_dropped() { return packets_dropped_; }

  int port() { return port_; }

 protected:
  virtual QuicDefaultPacketWriter* CreateWriter(int fd);

  virtual QuicDispatcher* CreateQuicDispatcher();

  const QuicConfig& config() const { return config_; }
  const QuicCryptoServerConfig& crypto_config() const { return crypto_config_; }
  EpollServer* epoll_server() { return &epoll_server_; }

  QuicDispatcher* dispatcher() { return dispatcher_.get(); }

  QuicVersionManager* version_manager() { return &version_manager_; }

  QuicHttpResponseCache* response_cache() { return response_cache_; }

  QuicHttpResponseProxy* quic_proxy_context() const {
    return quic_proxy_context_;
  }

  void set_silent_close(bool value) { silent_close_ = value; }

 private:
  friend class net::test::QuicServerPeer;

  // Initialize the internal state of the server.
  void Initialize();

  // Accepts data from the framer and demuxes clients to sessions.
  std::unique_ptr<QuicDispatcher> dispatcher_;
  // Frames incoming packets and hands them to the dispatcher.
  EpollServer epoll_server_;

  // The port the server is listening on.
  int port_;

  // Listening connection.  Also used for outbound client communication.
  int fd_;

  // If overflow_supported_ is true this will be the number of packets dropped
  // during the lifetime of the server.  This may overflow if enough packets
  // are dropped.
  QuicPacketCount packets_dropped_;

  // True if the kernel supports SO_RXQ_OVFL, the number of packets dropped
  // because the socket would otherwise overflow.
  bool overflow_supported_;

  // If true, do not call Shutdown on the dispatcher.  Connections will close
  // without sending a final connection close.
  bool silent_close_;

  // config_ contains non-crypto parameters that are negotiated in the crypto
  // handshake.
  QuicConfig config_;
  // crypto_config_ contains crypto parameters for the handshake.
  QuicCryptoServerConfig crypto_config_;
  // crypto_config_options_ contains crypto parameters for the handshake.
  QuicCryptoServerConfig::ConfigOptions crypto_config_options_;

  // Used to generate current supported versions.
  QuicVersionManager version_manager_;

  // Point to a QuicPacketReader object on the heap. The reader allocates more
  // space than allowed on the stack.
  std::unique_ptr<QuicPacketReader> packet_reader_;

  QuicHttpResponseCache* response_cache_;      // unowned.
  QuicHttpResponseProxy* quic_proxy_context_;  // unowned

  base::WeakPtrFactory<QuicServer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(QuicServer);
};

}  // namespace net

#endif  // NET_TOOLS_QUIC_QUIC_SERVER_H_
