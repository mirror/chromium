// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_NETWORK_STATS_H_
#define CHROME_BROWSER_NET_NETWORK_STATS_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "chrome/browser/io_thread.h"
#include "net/base/address_list.h"
#include "net/base/completion_callback.h"
#include "net/base/host_port_pair.h"
#include "net/base/host_resolver.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "net/base/test_data_stream.h"
#include "net/socket/socket.h"

namespace chrome_browser_net {

// This class is used for live experiment of network connectivity (either TCP or
// UDP) metrics. A small percentage of users participate in this experiment. All
// users (who are in the experiment) must have enabled "UMA upload".
//
// This class collects the following stats from users who have opted in.
// a) What percentage of users can get a message end-to-end to a UDP server?
// b) What percentage of users can get a message end-to-end to a TCP server?
// c) What is the latency for UDP and TCP.
// d) If connectivity failed, at what stage (Connect or Write or Read) did it
// fail?

class NetworkStats {
 public:
  enum Status {              // Used in HISTOGRAM_ENUMERATION.
    SUCCESS,                 // Successfully received bytes from the server.
    IP_STRING_PARSE_FAILED,  // Parsing of IP string failed.
    RESOLVE_FAILED,          // Host resolution failed.
    CONNECT_FAILED,          // Connection to the server failed.
    WRITE_FAILED,            // Sending an echo message to the server failed.
    READ_FAILED,             // Reading the reply from the server failed.
    READ_VERIFY_FAILED,      // Verification of data failed.
    STATUS_MAX,              // Bounding value.
  };

 protected:
  // Constructs an NetworkStats object that collects metrics for network
  // connectivity (either TCP or UDP).
  NetworkStats();
  virtual ~NetworkStats();

  // Initializes |finished_callback_| and the number of bytes to send to the
  // server. |finished_callback| is called when we are done with the test.
  // |finished_callback| is mainly useful for unittests.
  void Initialize(int bytes_to_send,
                  net::CompletionCallback* finished_callback);

  // This method is called after socket connection is completed. It will send
  // |bytes_to_send| bytes to |server| by calling SendData(). After successfully
  // sending data to the |server|, it calls ReadData() to read/verify the data
  // from the |server|. Returns true if successful.
  bool DoStart(int result);

  // Collects network connectivity stats. This is called when all the data from
  // server is read or when there is a failure during connect/read/write.
  virtual void Finish(Status status, int result) {}

  // This method is called from Finish() and calls |finished_callback_| callback
  // to indicate that the test has finished.
  void DoFinishCallback(int result);

  // Returns the number of bytes to be sent to the |server|.
  int load_size() const { return load_size_; }

  // Helper methods to get and set |socket_|.
  net::Socket* socket() { return socket_.get(); }
  void set_socket(net::Socket* socket);

  // Returns |start_time_| (used by histograms).
  base::TimeTicks start_time() const { return start_time_; }

 private:
  // Verifies the data and calls Finish() if there is an error or if all bytes
  // are read. Returns true if Finish() is called otherwise returns false.
  bool ReadComplete(int result);

  // Callbacks when an internal IO is completed.
  void OnReadComplete(int result);
  void OnWriteComplete(int result);

  // Reads data from server until an error occurs.
  void ReadData();

  // Sends data to server until an error occurs.
  int SendData();

  // The socket handle for this session.
  scoped_ptr<net::Socket> socket_;

  // The read buffer used to read data from the socket.
  scoped_refptr<net::IOBuffer> read_buffer_;

  // The write buffer used to write data to the socket.
  scoped_refptr<net::DrainableIOBuffer> write_buffer_;

  // Some counters for the session.
  int load_size_;
  int bytes_to_read_;
  int bytes_to_send_;

  // |stream_| is used to generate data to be sent to the server and it is also
  // used to verify the data received from the server.
  net::TestDataStream stream_;

  // Callback to call when data is read from the server.
  net::CompletionCallbackImpl<NetworkStats> read_callback_;

  // Callback to call when data is sent to the server.
  net::CompletionCallbackImpl<NetworkStats> write_callback_;

  // Callback to call when echo protocol is successefully finished or whenever
  // there is an error (this allows unittests to wait until echo protocol's
  // round trip is finished).
  net::CompletionCallback* finished_callback_;

  // The time when the session was started.
  base::TimeTicks start_time_;
};

class UDPStatsClient : public NetworkStats {
 public:
  // Constructs an UDPStatsClient object that collects metrics for UDP
  // connectivity.
  UDPStatsClient();
  virtual ~UDPStatsClient();

  // Starts the client, connecting to |server|.
  // Client will send |bytes_to_send| bytes to |server|.
  // When client has received all echoed bytes from the server, or
  // when an error occurs causing the client to stop, |Finish| will be
  // called with a net status code.
  // |Finish| will collect histogram stats.
  // Returns true if successful in starting the client.
  bool Start(const std::string& ip_str,
             int port,
             int bytes_to_send,
             net::CompletionCallback* callback);

 protected:
  // Allow tests to access our innards for testing purposes.
  friend class NetworkStatsTestUDP;

  // Collects stats for UDP connectivity. This is called when all the data from
  // server is read or when there is a failure during connect/read/write.
  virtual void Finish(Status status, int result);
};

class TCPStatsClient : public NetworkStats {
 public:
  // Constructs a TCPStatsClient object that collects metrics for TCP
  // connectivity.
  TCPStatsClient();
  virtual ~TCPStatsClient();

  // Starts the client, connecting to |server|.
  // Client will send |bytes_to_send| bytes.
  // When the client has received all echoed bytes from the server, or
  // when an error occurs causing the client to stop, |Finish| will be
  // called with a net status code.
  // |Finish| will collect histogram stats.
  // Returns true if successful in starting the client.
  bool Start(net::HostResolver* host_resolver,
             const net::HostPortPair& server,
             int bytes_to_send,
             net::CompletionCallback* callback);

 protected:
  // Allow tests to access our innards for testing purposes.
  friend class NetworkStatsTestTCP;

  // Collects stats for TCP connectivity. This is called when all the data from
  // server is read or when there is a failure during connect/read/write.
  virtual void Finish(Status status, int result);

 private:
  // Callback that is called when host resolution is completed.
  void OnResolveComplete(int result);

  // Called after host is resolved. Creates TCPClientSocket and connects to the
  // server.
  bool DoConnect(int result);

  // Callback that is called when connect is completed and calls DoStart() to
  // start the echo protocl.
  void OnConnectComplete(int result);

  // Callback to call when host resolution is completed.
  net::CompletionCallbackImpl<TCPStatsClient> resolve_callback_;

  // Callback to call when connect is completed.
  net::CompletionCallbackImpl<TCPStatsClient> connect_callback_;

  // HostResolver fills out the |addresses_| after host resolution is completed.
  net::AddressList addresses_;
};

// This collects the network connectivity stats for UDP and TCP for small
// percentage of users who are participating in the experiment. All users must
// have enabled "UMA upload". This method gets called only if UMA upload to the
// server has succeeded.
void CollectNetworkStats(const std::string& network_stats_server_url,
                         IOThread* io_thread);

}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_NETWORK_STATS_H_
