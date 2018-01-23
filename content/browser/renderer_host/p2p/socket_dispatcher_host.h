// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_DISPATCHER_HOST_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/sequenced_task_runner.h"
#include "content/browser/renderer_host/p2p/socket_host_throttler.h"
#include "content/common/p2p.mojom.h"
#include "content/common/p2p_socket_type.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/network_change_notifier.h"

namespace net {
class URLRequestContextGetter;
}

namespace rtc {
struct PacketOptions;
}

namespace content {

class P2PSocketHost;
class ResourceContext;

class P2PSocketDispatcherHost
    : public content::mojom::P2PSocketDispatcherHost,
      public net::NetworkChangeNotifier::NetworkChangeObserver,
      public IPC::Sender,
      public base::RefCountedDeleteOnSequence<P2PSocketDispatcherHost> {
 public:
  P2PSocketDispatcherHost(content::ResourceContext* resource_context,
                          net::URLRequestContextGetter* url_context);

  void Bind(mojom::P2PSocketDispatcherHostRequest request);

  bool Send(IPC::Message* msg) override;

  void OnError();

  // net::NetworkChangeNotifier::NetworkChangeObserver interface.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // Starts the RTP packet header dumping. Must be called on the IO thread.
  void StartRtpDump(
      bool incoming,
      bool outgoing,
      const RenderProcessHost::WebRtcRtpPacketCallback& packet_callback);

  // Stops the RTP packet header dumping. Must be called on the UI thread.
  void StopRtpDumpOnUIThread(bool incoming, bool outgoing);

 protected:
  ~P2PSocketDispatcherHost() override;

 private:
  friend class RefCountedDeleteOnSequence<P2PSocketDispatcherHost>;
  friend class base::DeleteHelper<P2PSocketDispatcherHost>;

  typedef std::map<int, std::unique_ptr<P2PSocketHost>> SocketsMap;

  class DnsRequest;

  P2PSocketHost* LookupSocket(int socket_id);

  // Handlers for the messages coming from the renderer.
  void StartNetworkNotifications() override;
  void StopNetworkNotifications() override;
  void GetHostAddress(const std::string& host_name,
                      int32_t request_id) override;
  void CreateSocket(P2PSocketType type,
                    int32_t socket_id,
                    const net::IPEndPoint& local_address,
                    const P2PPortRange& port_range,
                    const P2PHostAndIPEndPoint& remote_address) override;
  void AcceptIncomingTcpConnection(int32_t listen_socket_id,
                                   const net::IPEndPoint& remote_address,
                                   int32_t connected_socket_id) override;
  void Send2(int32_t socket_id,
             const net::IPEndPoint& socket_address,
             const std::vector<int8_t>& data,
             const rtc::PacketOptions& options,
             uint64_t packet_id) override;
  void SetOption(int32_t socket_id,
                 P2PSocketOption option,
                 int32_t value) override;
  void DestroySocket(int32_t socket_id) override;

  void DoGetNetworkList();
  void SendNetworkList(const net::NetworkInterfaceList& list,
                       const net::IPAddress& default_ipv4_local_address,
                       const net::IPAddress& default_ipv6_local_address);

  // This connects a UDP socket to a public IP address and gets local
  // address. Since it binds to the "any" address (0.0.0.0 or ::) internally, it
  // retrieves the default local address.
  net::IPAddress GetDefaultLocalAddress(int family);

  void OnAddressResolved(DnsRequest* request,
                         const net::IPAddressList& addresses);

  void StopRtpDumpOnIOThread(bool incoming, bool outgoing);

  content::ResourceContext* resource_context_;
  scoped_refptr<net::URLRequestContextGetter> url_context_;

  SocketsMap sockets_;

  bool monitoring_networks_;

  std::set<std::unique_ptr<DnsRequest>> dns_requests_;
  P2PMessageThrottler throttler_;

  net::IPAddress default_ipv4_local_address_;
  net::IPAddress default_ipv6_local_address_;

  bool dump_incoming_rtp_packet_;
  bool dump_outgoing_rtp_packet_;
  RenderProcessHost::WebRtcRtpPacketCallback packet_callback_;

  // Used to call DoGetNetworkList, which may briefly block since getting the
  // default local address involves creating a dummy socket.
  const scoped_refptr<base::SequencedTaskRunner> network_list_task_runner_;

  mojo::Binding<mojom::P2PSocketDispatcherHost> binding_;

  DISALLOW_COPY_AND_ASSIGN(P2PSocketDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_DISPATCHER_HOST_H_
