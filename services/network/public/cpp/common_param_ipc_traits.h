// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_

#include <string>

#include "base/pickle.h"
#include "ipc/ipc_param_traits.h"
#include "ipc/param_traits_macros.h"
#include "net/base/host_port_pair.h"
#include "net/http/http_request_headers.h"

#ifndef INTERNAL_SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_
#define INTERNAL_SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_

// services/network/public/cpp is currently packaged as a static library,
// so there's no need for export defines; it's linked directly into whatever
// other components need it.
// This redefinition is present for the IPC macros below.
#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT

namespace IPC {

template <>
struct ParamTraits<net::HostPortPair> {
  typedef net::HostPortPair param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct ParamTraits<net::HttpRequestHeaders> {
  typedef net::HttpRequestHeaders param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

}  // namespace IPC

#endif  // INTERNAL_SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_

IPC_STRUCT_TRAITS_BEGIN(net::HttpRequestHeaders::HeaderKeyValuePair)
  IPC_STRUCT_TRAITS_MEMBER(key)
  IPC_STRUCT_TRAITS_MEMBER(value)
IPC_STRUCT_TRAITS_END()

#endif  // SERVICES_NETWORK_PUBLIC_CPP_COMMON_PARAM_IPC_TRAITS_H_
