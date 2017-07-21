// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/net/x509_certificate_model.h"

#include <cert.h>
#include <cms.h>
#include <hasht.h>
#include <keyhi.h>  // SECKEY_DestroyPrivateKey
#include <keythi.h>  // SECKEYPrivateKey
#include <pk11pub.h>  // PK11_FindKeyByAnyCert
#include <seccomon.h>  // SECItem
#include <sechash.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <memory>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/third_party/mozilla_security_manager/nsNSSCertHelper.h"
#include "chrome/third_party/mozilla_security_manager/nsNSSCertificate.h"
#include "chrome/third_party/mozilla_security_manager/nsUsageArrayHelper.h"
#include "crypto/nss_util.h"
#include "crypto/scoped_nss_types.h"
#include "net/cert/x509_util_nss.h"

namespace psm = mozilla_security_manager;

namespace {

// Convert a char* return value from NSS into a std::string and free the NSS
// memory.  If the arg is NULL, an empty string will be returned instead.
std::string Stringize(char* nss_text, const std::string& alternative_text) {
  if (!nss_text)
    return alternative_text;

  std::string s = nss_text;
  PORT_Free(nss_text);
  return s;
}

// Hash a certificate using the given algorithm, return the result as a
// colon-seperated hex string.  The len specified is the number of bytes
// required for storing the raw fingerprint.
// (It's a bit redundant that the caller needs to specify len in addition to the
// algorithm, but given the limited uses, not worth fixing.)
std::string HashCert(CERTCertificate* cert, HASH_HashType algorithm, int len) {
  unsigned char fingerprint[HASH_LENGTH_MAX];

  DCHECK(NULL != cert->derCert.data);
  DCHECK_NE(0U, cert->derCert.len);
  DCHECK_LE(len, HASH_LENGTH_MAX);
  memset(fingerprint, 0, len);
  SECStatus rv = HASH_HashBuf(algorithm, fingerprint, cert->derCert.data,
                              cert->derCert.len);
  DCHECK_EQ(rv, SECSuccess);
  return x509_certificate_model::ProcessRawBytes(fingerprint, len);
}

std::string ProcessSecAlgorithmInternal(SECAlgorithmID* algorithm_id) {
  return psm::GetOIDText(&algorithm_id->algorithm);
}

std::string ProcessExtension(
    const std::string& critical_label,
    const std::string& non_critical_label,
    CERTCertExtension* extension) {
  std::string criticality =
      extension->critical.data && extension->critical.data[0] ?
          critical_label : non_critical_label;
  return criticality + "\n" + psm::ProcessExtensionData(extension);
}

std::string GetNickname(CERTCertificate* cert_handle) {
  std::string name;
  if (cert_handle->nickname) {
    name = cert_handle->nickname;
    // Hack copied from mozilla: Cut off text before first :, which seems to
    // just be the token name.
    size_t colon_pos = name.find(':');
    if (colon_pos != std::string::npos)
      name = name.substr(colon_pos + 1);
  }
  return name;
}

std::string DecodeAVAValue(CERTAVA* ava) {
  SECItem* decode_item = CERT_DecodeAVAValue(&ava->value);
  if (!decode_item)
    return std::string();
  std::string value(reinterpret_cast<char*>(decode_item->data),
                    decode_item->len);
  SECITEM_FreeItem(decode_item, PR_TRUE);
  return value;
}

std::string GetDisplayName(CERTName* name) {
  std::string result = Stringize(CERT_GetCommonName(name), "");
  if (!result.empty())
    return result;

  // XXX is all this necesary? can it just use CERT_GetOrgName etc?
  CERTAVA* ou_ava = nullptr;

  CERTRDN** rdns = name->rdns;
  for (size_t rdn = 0; rdns[rdn]; ++rdn) {
    CERTAVA** avas = rdns[rdn]->avas;
    for (size_t pair = 0; avas[pair] != 0; ++pair) {
      SECOidTag tag = CERT_GetAVATag(avas[pair]);
      if (tag == SEC_OID_AVA_ORGANIZATION_NAME)
        return DecodeAVAValue(avas[pair]);
      if (tag == SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME && !ou_ava)
        ou_ava = avas[pair];
    }
  }
  if (ou_ava)
    return DecodeAVAValue(ou_ava);
  return std::string();
}

////////////////////////////////////////////////////////////////////////////////
// NSS certificate export functions.

struct NSSCMSMessageDeleter {
  inline void operator()(NSSCMSMessage* x) const {
    NSS_CMSMessage_Destroy(x);
  }
};
typedef std::unique_ptr<NSSCMSMessage, NSSCMSMessageDeleter>
    ScopedNSSCMSMessage;

struct FreeNSSCMSSignedData {
  inline void operator()(NSSCMSSignedData* x) const {
    NSS_CMSSignedData_Destroy(x);
  }
};
typedef std::unique_ptr<NSSCMSSignedData, FreeNSSCMSSignedData>
    ScopedNSSCMSSignedData;

}  // namespace

namespace x509_certificate_model {

using std::string;

string GetCertNameOrNickname(CERTCertificate* cert_handle) {
  string name = ProcessIDN(
      Stringize(CERT_GetCommonName(&cert_handle->subject), std::string()));
  if (!name.empty())
    return name;
  return GetNickname(cert_handle);
}

string GetTokenName(CERTCertificate* cert_handle) {
  return psm::GetCertTokenName(cert_handle);
}

string GetVersion(CERTCertificate* cert_handle) {
  // If the version field is omitted from the certificate, the default
  // value is v1(0).
  unsigned long version = 0;
  if (cert_handle->version.len == 0 ||
      SEC_ASN1DecodeInteger(&cert_handle->version, &version) == SECSuccess) {
    return base::Uint64ToString(base::strict_cast<uint64_t>(version + 1));
  }
  return std::string();
}

net::CertType GetType(CERTCertificate* cert_handle) {
  return psm::GetCertType(cert_handle);
}

void GetUsageStrings(CERTCertificate* cert_handle,
                     std::vector<string>* usages) {
  psm::GetCertUsageStrings(cert_handle, usages);
}

string GetSerialNumberHexified(CERTCertificate* cert_handle,
                               const string& alternative_text) {
  return Stringize(CERT_Hexify(&cert_handle->serialNumber, true),
                   alternative_text);
}

string GetIssuerCommonName(CERTCertificate* cert_handle,
                           const string& alternative_text) {
  return Stringize(CERT_GetCommonName(&cert_handle->issuer), alternative_text);
}

string GetIssuerOrgName(CERTCertificate* cert_handle,
                        const string& alternative_text) {
  return Stringize(CERT_GetOrgName(&cert_handle->issuer), alternative_text);
}

string GetIssuerOrgUnitName(CERTCertificate* cert_handle,
                            const string& alternative_text) {
  return Stringize(CERT_GetOrgUnitName(&cert_handle->issuer), alternative_text);
}

string GetSubjectOrgName(CERTCertificate* cert_handle,
                         const string& alternative_text) {
  return Stringize(CERT_GetOrgName(&cert_handle->subject), alternative_text);
}

string GetSubjectOrgUnitName(CERTCertificate* cert_handle,
                             const string& alternative_text) {
  return Stringize(CERT_GetOrgUnitName(&cert_handle->subject),
                   alternative_text);
}

string GetSubjectCommonName(CERTCertificate* cert_handle,
                            const string& alternative_text) {
  return Stringize(CERT_GetCommonName(&cert_handle->subject), alternative_text);
}

bool GetTimes(CERTCertificate* cert_handle,
              base::Time* issued,
              base::Time* expires) {
  return net::x509_util::GetValidityTimes(cert_handle, issued, expires);
}

string GetTitle(CERTCertificate* cert_handle) {
  return psm::GetCertTitle(cert_handle);
}

string GetIssuerName(CERTCertificate* cert_handle) {
  return psm::ProcessName(&cert_handle->issuer);
}

string GetSubjectName(CERTCertificate* cert_handle) {
  return psm::ProcessName(&cert_handle->subject);
}

std::string GetIssuerDisplayName(CERTCertificate* cert_handle) {
  return GetDisplayName(&cert_handle->issuer);
}

std::string GetSubjectDisplayName(CERTCertificate* cert_handle) {
  return GetDisplayName(&cert_handle->subject);
}

void GetExtensions(const string& critical_label,
                   const string& non_critical_label,
                   CERTCertificate* cert_handle,
                   Extensions* extensions) {
  if (cert_handle->extensions) {
    for (size_t i = 0; cert_handle->extensions[i] != NULL; ++i) {
      Extension extension;
      extension.name = psm::GetOIDText(&cert_handle->extensions[i]->id);
      extension.value = ProcessExtension(
          critical_label, non_critical_label, cert_handle->extensions[i]);
      extensions->push_back(extension);
    }
  }
}

string HashCertSHA256(CERTCertificate* cert_handle) {
  return HashCert(cert_handle, HASH_AlgSHA256, SHA256_LENGTH);
}

string HashCertSHA1(CERTCertificate* cert_handle) {
  return HashCert(cert_handle, HASH_AlgSHA1, SHA1_LENGTH);
}

string GetCMSString(const net::ScopedCERTCertificateVector& cert_chain,
                    size_t start,
                    size_t end) {
  crypto::ScopedPLArenaPool arena(PORT_NewArena(1024));
  DCHECK(arena.get());

  ScopedNSSCMSMessage message(NSS_CMSMessage_Create(arena.get()));
  DCHECK(message.get());

  // First, create SignedData with the certificate only (no chain).
  ScopedNSSCMSSignedData signed_data(NSS_CMSSignedData_CreateCertsOnly(
      message.get(), cert_chain[start].get(), PR_FALSE));
  if (!signed_data.get()) {
    DLOG(ERROR) << "NSS_CMSSignedData_Create failed";
    return std::string();
  }
  // Add the rest of the chain (if any).
  for (size_t i = start + 1; i < end; ++i) {
    if (NSS_CMSSignedData_AddCertificate(signed_data.get(),
                                         cert_chain[i].get()) != SECSuccess) {
      DLOG(ERROR) << "NSS_CMSSignedData_AddCertificate failed on " << i;
      return std::string();
    }
  }

  NSSCMSContentInfo *cinfo = NSS_CMSMessage_GetContentInfo(message.get());
  if (NSS_CMSContentInfo_SetContent_SignedData(
      message.get(), cinfo, signed_data.get()) == SECSuccess) {
    ignore_result(signed_data.release());
  } else {
    DLOG(ERROR) << "NSS_CMSMessage_GetContentInfo failed";
    return std::string();
  }

  SECItem cert_p7 = { siBuffer, NULL, 0 };
  NSSCMSEncoderContext *ecx = NSS_CMSEncoder_Start(message.get(), NULL, NULL,
                                                   &cert_p7, arena.get(), NULL,
                                                   NULL, NULL, NULL, NULL,
                                                   NULL);
  if (!ecx) {
    DLOG(ERROR) << "NSS_CMSEncoder_Start failed";
    return std::string();
  }

  if (NSS_CMSEncoder_Finish(ecx) != SECSuccess) {
    DLOG(ERROR) << "NSS_CMSEncoder_Finish failed";
    return std::string();
  }

  return string(reinterpret_cast<const char*>(cert_p7.data), cert_p7.len);
}

string ProcessSecAlgorithmSignature(CERTCertificate* cert_handle) {
  return ProcessSecAlgorithmInternal(&cert_handle->signature);
}

string ProcessSecAlgorithmSubjectPublicKey(CERTCertificate* cert_handle) {
  return ProcessSecAlgorithmInternal(
      &cert_handle->subjectPublicKeyInfo.algorithm);
}

string ProcessSecAlgorithmSignatureWrap(CERTCertificate* cert_handle) {
  return ProcessSecAlgorithmInternal(
      &cert_handle->signatureWrap.signatureAlgorithm);
}

string ProcessSubjectPublicKeyInfo(CERTCertificate* cert_handle) {
  return psm::ProcessSubjectPublicKeyInfo(&cert_handle->subjectPublicKeyInfo);
}

string ProcessRawBitsSignatureWrap(CERTCertificate* cert_handle) {
  return ProcessRawBits(cert_handle->signatureWrap.signature.data,
                        cert_handle->signatureWrap.signature.len);
}

}  // namespace x509_certificate_model
