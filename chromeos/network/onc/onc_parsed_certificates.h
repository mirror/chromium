// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_ONC_ONC_PARSED_CERTIFICATES_H_
#define CHROMEOS_NETWORK_ONC_ONC_PARSED_CERTIFICATES_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromeos/chromeos_export.h"

namespace base {
class Value;
}  // namespace base

namespace net {
class X509Certificate;
}

namespace chromeos {
namespace onc {

class CHROMEOS_EXPORT OncParsedCertificates {
 public:
  enum class CertificateType { kServer, kAuthority, kClient };
  class Certificate {
   public:
    CertificateType type() const { return type_; }
    const std::string& guid() const { return guid_; }

   protected:
    Certificate(CertificateType type, const std::string& guid);
    Certificate(const Certificate& other);
    Certificate& operator=(const Certificate& other);
    Certificate(Certificate&& other);
    ~Certificate();

   private:
    CertificateType type_;
    std::string guid_;
  };

  class ServerOrAuthorityCertificate : public Certificate {
   public:
    ServerOrAuthorityCertificate(
        CertificateType type,
        const std::string& guid,
        const scoped_refptr<net::X509Certificate>& certificate,
        bool web_trust_requested);
    ServerOrAuthorityCertificate(const ServerOrAuthorityCertificate& other);
    ServerOrAuthorityCertificate& operator=(
        const ServerOrAuthorityCertificate& other);
    ServerOrAuthorityCertificate(ServerOrAuthorityCertificate&& other);
    ~ServerOrAuthorityCertificate();

    const scoped_refptr<net::X509Certificate>& certificate() const {
      return certificate_;
    }
    bool web_trust_requested() const { return web_trust_requested_; }

   private:
    scoped_refptr<net::X509Certificate> certificate_;
    bool web_trust_requested_;
  };

  class ClientCertificate : public Certificate {
   public:
    ClientCertificate(const std::string& guid, const std::string& pkcs12_data);
    ClientCertificate(const ClientCertificate& other);
    ClientCertificate& operator=(const ClientCertificate& other);
    ClientCertificate(ClientCertificate&& other);
    ~ClientCertificate();

    const std::string& pkcs12_data() const { return pkcs12_data_; }

   private:
    std::string pkcs12_data_;
  };

  OncParsedCertificates();
  explicit OncParsedCertificates(const base::Value& onc_certificates);
  OncParsedCertificates(const OncParsedCertificates& other);
  OncParsedCertificates(OncParsedCertificates&& other);
  OncParsedCertificates& operator=(const OncParsedCertificates& other);
  ~OncParsedCertificates();

  const std::vector<ServerOrAuthorityCertificate>&
  server_or_authority_certificates() const {
    return server_or_authority_certificates_;
  }
  const std::vector<ClientCertificate>& client_certificates() const {
    return client_certificates_;
  }
  bool has_error() const { return has_error_; }

  bool HasSameServerOrAuthorityCertificates(OncParsedCertificates* other) const;
  bool HasSameClientCertificates(OncParsedCertificates* other) const;

 private:
  bool ParseCertificate(const base::Value& onc_certificate);
  bool ParseServerOrCaCertificate(CertificateType type,
                                  const std::string& guid,
                                  const base::Value& onc_certificate);
  bool ParseClientCertificate(const std::string& guid,
                              const base::Value& onc_certificate);

  std::vector<ServerOrAuthorityCertificate> server_or_authority_certificates_;
  std::vector<ClientCertificate> client_certificates_;
  bool has_error_ = false;
};

}  // namespace onc
}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_ONC_ONC_PARSED_CERTIFICATES_H_
