// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_SSL_INFO_IPC_TRAITS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_SSL_INFO_IPC_TRAITS_H_

#include <string>

#include "base/pickle.h"
#include "ipc/ipc_param_traits.h"
#include "ipc/param_traits_macros.h"
#include "net/cert/ct_policy_status.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/signed_certificate_timestamp_and_status.h"
#include "net/ssl/ssl_info.h"
#include "services/network/public/cpp/export.h"

#ifndef INTERNAL_SERVICES_NETWORK_PUBLIC_CPP_SSL_INFO_IPC_TRAITS_H_
#define INTERNAL_SERVICES_NETWORK_PUBLIC_CPP_SSL_INFO_IPC_TRAITS_H_

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT NETWORK_PUBLIC_CPP_EXPORT

namespace IPC {

template <>
struct IPC_MESSAGE_EXPORT ParamTraits<net::HashValue> {
  typedef net::HashValue param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct IPC_MESSAGE_EXPORT ParamTraits<net::SSLInfo> {
  typedef net::SSLInfo param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct IPC_MESSAGE_EXPORT
    ParamTraits<scoped_refptr<net::ct::SignedCertificateTimestamp>> {
  typedef scoped_refptr<net::ct::SignedCertificateTimestamp> param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

}  // namespace IPC

#endif  // INTERNAL_SERVICES_NETWORK_PUBLIC_CPP_SSL_INFO_IPC_TRAITS_H_

IPC_ENUM_TRAITS_MAX_VALUE(
    net::ct::CertPolicyCompliance,
    net::ct::CertPolicyCompliance::CERT_POLICY_BUILD_NOT_TIMELY)
IPC_ENUM_TRAITS_MAX_VALUE(net::OCSPVerifyResult::ResponseStatus,
                          net::OCSPVerifyResult::PARSE_RESPONSE_DATA_ERROR)
IPC_ENUM_TRAITS_MAX_VALUE(net::OCSPRevocationStatus,
                          net::OCSPRevocationStatus::UNKNOWN)

IPC_ENUM_TRAITS_MAX_VALUE(net::ct::SCTVerifyStatus, net::ct::SCT_STATUS_MAX)

IPC_ENUM_TRAITS_MAX_VALUE(net::SSLInfo::HandshakeType,
                          net::SSLInfo::HANDSHAKE_FULL)
IPC_ENUM_TRAITS_MAX_VALUE(net::TokenBindingParam, net::TB_PARAM_ECDSAP256)

IPC_STRUCT_TRAITS_BEGIN(net::SignedCertificateTimestampAndStatus)
  IPC_STRUCT_TRAITS_MEMBER(sct)
  IPC_STRUCT_TRAITS_MEMBER(status)
IPC_STRUCT_TRAITS_END()

#endif  // SERVICES_NETWORK_PUBLIC_CPP_SSL_INFO_IPC_TRAITS_H_
