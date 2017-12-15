// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/common_param_ipc_traits.h"

#include "ipc/ipc_message_utils.h"
#include "net/http/http_util.h"

namespace IPC {

void ParamTraits<net::HostPortPair>::Write(base::Pickle* m,
                                           const param_type& p) {
  WriteParam(m, p.host());
  WriteParam(m, p.port());
}

bool ParamTraits<net::HostPortPair>::Read(const base::Pickle* m,
                                          base::PickleIterator* iter,
                                          param_type* r) {
  std::string host;
  uint16_t port;
  if (!ReadParam(m, iter, &host) || !ReadParam(m, iter, &port))
    return false;

  r->set_host(host);
  r->set_port(port);
  return true;
}

void ParamTraits<net::HostPortPair>::Log(const param_type& p, std::string* l) {
  l->append(p.ToString());
}

void ParamTraits<net::HttpRequestHeaders>::Write(base::Pickle* m,
                                                 const param_type& p) {
  WriteParam(m, static_cast<int>(p.GetHeaderVector().size()));
  for (size_t i = 0; i < p.GetHeaderVector().size(); ++i)
    WriteParam(m, p.GetHeaderVector()[i]);
}

bool ParamTraits<net::HttpRequestHeaders>::Read(const base::Pickle* m,
                                                base::PickleIterator* iter,
                                                param_type* r) {
  // Sanity check.
  int size;
  if (!iter->ReadLength(&size))
    return false;
  for (int i = 0; i < size; ++i) {
    net::HttpRequestHeaders::HeaderKeyValuePair pair;
    if (!ReadParam(m, iter, &pair) ||
        !net::HttpUtil::IsValidHeaderName(pair.key) ||
        !net::HttpUtil::IsValidHeaderValue(pair.value))
      return false;
    r->SetHeader(pair.key, pair.value);
  }
  return true;
}

void ParamTraits<net::HttpRequestHeaders>::Log(const param_type& p,
                                               std::string* l) {
  l->append(p.ToString());
}

}  // namespace IPC

// Generation of IPC definitions.

// Generate constructors.
#undef SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_
#include "ipc/struct_constructor_macros.h"
#include "common_param_ipc_traits.h"

// Generate destructors.
#undef SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_
#include "ipc/struct_destructor_macros.h"
#include "common_param_ipc_traits.h"

// Generate param traits write methods.
#undef SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#include "common_param_ipc_traits.h"
}  // namespace IPC

// Generate param traits read methods.
#undef SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#include "common_param_ipc_traits.h"
}  // namespace IPC

// Generate param traits log methods.
#undef SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#include "common_param_ipc_traits.h"
}  // namespace IPC
