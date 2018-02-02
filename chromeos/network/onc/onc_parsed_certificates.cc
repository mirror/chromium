// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_parsed_certificates.h"

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/optional.h"
#include "base/values.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/onc/onc_constants.h"
#include "net/cert/x509_certificate.h"

namespace chromeos {
namespace onc {

namespace {

// Returns true if the certificate described by |onc_certificate| requests web
// trust.
bool HasWebTrustFlag(const base::Value& onc_certificate) {
  DCHECK(onc_certificate.is_dict());

  bool web_trust_flag = false;
  const base::Value* trust_list = onc_certificate.FindKeyOfType(
      ::onc::certificate::kTrustBits, base::Value::Type::LIST);
  if (trust_list) {
    for (const base::Value& trust_entry : trust_list->GetList()) {
      DCHECK(trust_entry.is_string());

      if (trust_entry.GetString() == ::onc::certificate::kWeb) {
        // "Web" implies that the certificate is to be trusted for SSL
        // identification.
        web_trust_flag = true;
      } else {
        // Trust bits should only increase trust and never restrict. Thus,
        // ignoring unknown bits should be safe.
        LOG(WARNING) << "Certificate contains unknown trust type "
                     << trust_entry.GetString();
      }
    }
  }

  return web_trust_flag;
}

// Converts the ONC string certificate type into the CertificateType enum.
// Returns |base::nullopt| if the certificate type was not understood.
base::Optional<OncParsedCertificates::CertificateType> GetCertTypeAsEnum(
    const std::string& cert_type) {
  if (cert_type == ::onc::certificate::kServer) {
    return base::make_optional(OncParsedCertificates::CertificateType::kServer);
  }

  if (cert_type == ::onc::certificate::kAuthority) {
    return base::make_optional(
        OncParsedCertificates::CertificateType::kAuthority);
  }

  if (cert_type == ::onc::certificate::kClient) {
    return base::make_optional(OncParsedCertificates::CertificateType::kClient);
  }

  return base::nullopt;
}

}  // namespace

OncParsedCertificates::Certificate::Certificate(CertificateType type,
                                                const std::string& guid)
    : type_(type), guid_(guid) {}

OncParsedCertificates::Certificate::Certificate(const Certificate& other) =
    default;

OncParsedCertificates::Certificate& OncParsedCertificates::Certificate::
operator=(const Certificate& other) = default;

OncParsedCertificates::Certificate::Certificate(Certificate&& other) = default;

OncParsedCertificates::Certificate::~Certificate() = default;

OncParsedCertificates::ServerOrAuthorityCertificate::
    ServerOrAuthorityCertificate(
        CertificateType type,
        const std::string& guid,
        const scoped_refptr<net::X509Certificate>& certificate,
        bool web_trust_requested)
    : Certificate(type, guid),
      certificate_(certificate),
      web_trust_requested_(web_trust_requested) {}

OncParsedCertificates::ServerOrAuthorityCertificate::
    ServerOrAuthorityCertificate(const ServerOrAuthorityCertificate& other) =
        default;

OncParsedCertificates::ServerOrAuthorityCertificate&
OncParsedCertificates::ServerOrAuthorityCertificate::operator=(
    const ServerOrAuthorityCertificate& other) = default;

OncParsedCertificates::ServerOrAuthorityCertificate::
    ServerOrAuthorityCertificate(ServerOrAuthorityCertificate&& other) =
        default;

OncParsedCertificates::ServerOrAuthorityCertificate::
    ~ServerOrAuthorityCertificate() = default;

bool OncParsedCertificates::ServerOrAuthorityCertificate::operator==(
    const ServerOrAuthorityCertificate& other) const {
  if (guid() != other.guid())
    return false;

  if (type() != other.type())
    return false;

  if (!certificate()->Equals(other.certificate().get()))
    return false;

  if (web_trust_requested() != other.web_trust_requested())
    return false;

  return true;
}

bool OncParsedCertificates::ServerOrAuthorityCertificate::operator!=(
    const ServerOrAuthorityCertificate& other) const {
  return !(*this == other);
}

OncParsedCertificates::ClientCertificate::ClientCertificate(
    const std::string& guid,
    const std::string& pkcs12_data)
    : Certificate(CertificateType::kClient, guid), pkcs12_data_(pkcs12_data) {}

OncParsedCertificates::ClientCertificate::ClientCertificate(
    const ClientCertificate& other) = default;

OncParsedCertificates::ClientCertificate&
OncParsedCertificates::ClientCertificate::operator=(
    const ClientCertificate& other) = default;

OncParsedCertificates::ClientCertificate::ClientCertificate(
    ClientCertificate&& other) = default;

OncParsedCertificates::ClientCertificate::~ClientCertificate() = default;

bool OncParsedCertificates::ClientCertificate::operator==(
    const ClientCertificate& other) const {
  if (guid() != other.guid())
    return false;

  if (pkcs12_data() != other.pkcs12_data())
    return false;

  return true;
}

bool OncParsedCertificates::ClientCertificate::operator!=(
    const ClientCertificate& other) const {
  return !(*this == other);
}

OncParsedCertificates::OncParsedCertificates()
    : OncParsedCertificates(base::ListValue()) {}

OncParsedCertificates::OncParsedCertificates(
    const base::Value& onc_certificates) {
  DCHECK(onc_certificates.is_list());
  if (!onc_certificates.is_list()) {
    has_error_ = true;
    return;
  }

  for (size_t i = 0; i < onc_certificates.GetList().size(); ++i) {
    const base::Value& onc_certificate = onc_certificates.GetList().at(i);
    DCHECK(onc_certificate.is_dict());

    VLOG(2) << "Parsing certificate at index " << i << ": " << onc_certificate;

    if (!ParseCertificate(onc_certificate)) {
      has_error_ = true;
      LOG(ERROR) << "Cannot parse certificate at index " << i;
    } else {
      VLOG(2) << "Successfully parsed certificate at index " << i;
    }
  }
}

OncParsedCertificates::OncParsedCertificates(
    const OncParsedCertificates& other) = default;

OncParsedCertificates::OncParsedCertificates(OncParsedCertificates&& other) =
    default;

OncParsedCertificates& OncParsedCertificates::operator=(
    const OncParsedCertificates& other) = default;

OncParsedCertificates::~OncParsedCertificates() = default;

bool OncParsedCertificates::ParseCertificate(
    const base::Value& onc_certificate) {
  const base::Value* guid_key = onc_certificate.FindKeyOfType(
      ::onc::certificate::kGUID, base::Value::Type::STRING);
  DCHECK(guid_key);
  std::string guid = guid_key->GetString();

  const base::Value* type_key = onc_certificate.FindKeyOfType(
      ::onc::certificate::kType, base::Value::Type::STRING);
  DCHECK(type_key);
  base::Optional<CertificateType> type_opt =
      GetCertTypeAsEnum(type_key->GetString());
  if (!type_opt)
    return false;

  CertificateType cert_type = type_opt.value();
  if (cert_type == CertificateType::kServer ||
      cert_type == CertificateType::kAuthority) {
    return ParseServerOrCaCertificate(cert_type, guid, onc_certificate);
  } else if (cert_type == CertificateType::kClient) {
    return ParseClientCertificate(guid, onc_certificate);
  }
  return false;
}

bool OncParsedCertificates::ParseServerOrCaCertificate(
    CertificateType cert_type,
    const std::string& guid,
    const base::Value& onc_certificate) {
  bool web_trust_requested = HasWebTrustFlag(onc_certificate);
  const base::Value* x509_data_key = onc_certificate.FindKeyOfType(
      ::onc::certificate::kX509, base::Value::Type::STRING);
  if (!x509_data_key || x509_data_key->GetString().empty()) {
    LOG(ERROR) << "Certificate missing " << ::onc::certificate::kX509
               << " certificate data.";
    return false;
  }

  std::string certificate_der_data = DecodePEM(x509_data_key->GetString());
  if (certificate_der_data.empty()) {
    LOG(ERROR) << "Unable to create certificate from PEM encoding.";
    return false;
  }

  scoped_refptr<net::X509Certificate> certificate =
      net::X509Certificate::CreateFromBytes(certificate_der_data.data(),
                                            certificate_der_data.length());
  if (!certificate) {
    LOG(ERROR) << "Unable to create certificate from PEM encoding.";
    return false;
  }

  server_or_authority_certificates_.push_back(ServerOrAuthorityCertificate(
      cert_type, guid, certificate, web_trust_requested));
  return true;
}

bool OncParsedCertificates::ParseClientCertificate(
    const std::string& guid,
    const base::Value& onc_certificate) {
  const base::Value* base64_pkcs12_data_key = onc_certificate.FindKeyOfType(
      ::onc::certificate::kPKCS12, base::Value::Type::STRING);
  if (!base64_pkcs12_data_key || base64_pkcs12_data_key->GetString().empty()) {
    LOG(ERROR) << "PKCS12 data is missing for client certificate.";
    return false;
  }

  std::string pkcs12_data;
  if (!base::Base64Decode(base64_pkcs12_data_key->GetString(), &pkcs12_data)) {
    LOG(ERROR) << "Unable to base64 decode PKCS#12 data: \""
               << base64_pkcs12_data_key->GetString() << "\".";
    return false;
  }

  client_certificates_.push_back(ClientCertificate(guid, pkcs12_data));
  return true;
}

}  // namespace onc
}  // namespace chromeos
