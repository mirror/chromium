// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/proxy_resolver_struct_traits.h"

#include "base/logging.h"
#include "net/base/host_port_pair.h"
#include "net/proxy/proxy_info.h"
#include "net/proxy/proxy_server.h"

namespace mojo {

content::mojom::ProxyScheme
EnumTraits<content::mojom::ProxyScheme, net::ProxyServer::Scheme>::ToMojom(
    net::ProxyServer::Scheme scheme) {
  using net::ProxyServer;
  switch (scheme) {
    case ProxyServer::SCHEME_INVALID:
      return content::mojom::ProxyScheme::INVALID;
    case ProxyServer::SCHEME_DIRECT:
      return content::mojom::ProxyScheme::DIRECT;
    case ProxyServer::SCHEME_HTTP:
      return content::mojom::ProxyScheme::HTTP;
    case ProxyServer::SCHEME_SOCKS4:
      return content::mojom::ProxyScheme::SOCKS4;
    case ProxyServer::SCHEME_SOCKS5:
      return content::mojom::ProxyScheme::SOCKS5;
    case ProxyServer::SCHEME_HTTPS:
      return content::mojom::ProxyScheme::HTTPS;
    case ProxyServer::SCHEME_QUIC:
      return content::mojom::ProxyScheme::QUIC;
  }
  NOTREACHED();
  return content::mojom::ProxyScheme::INVALID;
}

bool EnumTraits<content::mojom::ProxyScheme, net::ProxyServer::Scheme>::
    FromMojom(content::mojom::ProxyScheme scheme,
              net::ProxyServer::Scheme* out) {
  using net::ProxyServer;
  switch (scheme) {
    case content::mojom::ProxyScheme::INVALID:
      *out = ProxyServer::SCHEME_INVALID;
      return true;
    case content::mojom::ProxyScheme::DIRECT:
      *out = ProxyServer::SCHEME_DIRECT;
      return true;
    case content::mojom::ProxyScheme::HTTP:
      *out = ProxyServer::SCHEME_HTTP;
      return true;
    case content::mojom::ProxyScheme::SOCKS4:
      *out = ProxyServer::SCHEME_SOCKS4;
      return true;
    case content::mojom::ProxyScheme::SOCKS5:
      *out = ProxyServer::SCHEME_SOCKS5;
      return true;
    case content::mojom::ProxyScheme::HTTPS:
      *out = ProxyServer::SCHEME_HTTPS;
      return true;
    case content::mojom::ProxyScheme::QUIC:
      *out = ProxyServer::SCHEME_QUIC;
      return true;
  }
  return false;
}

base::StringPiece
StructTraits<content::mojom::ProxyServerDataView, net::ProxyServer>::host(
    const net::ProxyServer& s) {
  if (s.scheme() == net::ProxyServer::SCHEME_DIRECT ||
      s.scheme() == net::ProxyServer::SCHEME_INVALID) {
    return base::StringPiece();
  }
  return s.host_port_pair().host();
}

uint16_t StructTraits<content::mojom::ProxyServerDataView,
                      net::ProxyServer>::port(const net::ProxyServer& s) {
  if (s.scheme() == net::ProxyServer::SCHEME_DIRECT ||
      s.scheme() == net::ProxyServer::SCHEME_INVALID) {
    return 0;
  }
  return s.host_port_pair().port();
}

bool StructTraits<content::mojom::ProxyServerDataView, net::ProxyServer>::Read(
    content::mojom::ProxyServerDataView data,
    net::ProxyServer* out) {
  net::ProxyServer::Scheme scheme;
  if (!data.ReadScheme(&scheme))
    return false;

  base::StringPiece host;
  if (!data.ReadHost(&host))
    return false;

  if ((scheme == net::ProxyServer::SCHEME_DIRECT ||
       scheme == net::ProxyServer::SCHEME_INVALID) &&
      (!host.empty() || data.port())) {
    return false;
  }

  *out = net::ProxyServer(scheme,
                          net::HostPortPair(host.as_string(), data.port()));
  return true;
}

bool StructTraits<content::mojom::ProxyInfoDataView, net::ProxyInfo>::Read(
    content::mojom::ProxyInfoDataView data,
    net::ProxyInfo* out) {
  std::vector<net::ProxyServer> proxy_servers;
  if (!data.ReadProxyServers(&proxy_servers))
    return false;

  net::ProxyList proxy_list;
  for (const auto& server : proxy_servers)
    proxy_list.AddProxyServer(server);

  out->UseProxyList(proxy_list);
  return true;
}

}  // namespace mojo
