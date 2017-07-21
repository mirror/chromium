// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/x509_util_nss.h"

#include <cert.h>  // Must be included before certdb.h
#include <certdb.h>
#include <cryptohi.h>
#include <nss.h>
#include <pk11pub.h>
#include <prerror.h>
#include <secder.h>
#include <sechash.h>
#include <secmod.h>
#include <secport.h>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "crypto/nss_util.h"
#include "crypto/scoped_nss_types.h"
#include "third_party/boringssl/src/include/openssl/pool.h"

// XXX double check that everything does error checking

namespace net {

namespace {

// Microsoft User Principal Name: 1.3.6.1.4.1.311.20.2.3
const uint8_t kUpnOid[] = {0x2b, 0x6,  0x1,  0x4, 0x1,
                           0x82, 0x37, 0x14, 0x2, 0x3};

// Generates a unique nickname for |slot|, returning |nickname| if it is
// already unique.
//
// Note: The nickname returned will NOT include the token name, thus the
// token name must be prepended if calling an NSS function that expects
// <token>:<nickname>.
// TODO(gspencer): Internationalize this: it's wrong to hard-code English.
std::string GetUniqueNicknameForSlot(const std::string& nickname,
                                     const SECItem* subject,
                                     PK11SlotInfo* slot) {
  int index = 2;
  std::string new_name = nickname;
  std::string temp_nickname = new_name;
  std::string token_name;

  if (!slot)
    return new_name;

  if (!PK11_IsInternalKeySlot(slot)) {
    token_name.assign(PK11_GetTokenName(slot));
    token_name.append(":");

    temp_nickname = token_name + new_name;
  }

  while (SEC_CertNicknameConflict(temp_nickname.c_str(),
                                  const_cast<SECItem*>(subject),
                                  CERT_GetDefaultCertDB())) {
    base::SStringPrintf(&new_name, "%s #%d", nickname.c_str(), index++);
    temp_nickname = token_name + new_name;
  }

  return new_name;
}

// The default nickname of the certificate, based on the certificate type
// passed in.
std::string GetDefaultNickname(CERTCertificate* nss_cert, CertType type) {
  std::string result;
  if (type == USER_CERT && nss_cert->slot) {
    // Find the private key for this certificate and see if it has a
    // nickname.  If there is a private key, and it has a nickname, then
    // return that nickname.
    SECKEYPrivateKey* private_key =
        PK11_FindPrivateKeyFromCert(nss_cert->slot, nss_cert, NULL /*wincx*/);
    if (private_key) {
      char* private_key_nickname = PK11_GetPrivateKeyNickname(private_key);
      if (private_key_nickname) {
        result = private_key_nickname;
        PORT_Free(private_key_nickname);
        SECKEY_DestroyPrivateKey(private_key);
        return result;
      }
      SECKEY_DestroyPrivateKey(private_key);
    }
  }

  switch (type) {
    case CA_CERT: {
      char* nickname = CERT_MakeCANickname(nss_cert);
      result = nickname;
      PORT_Free(nickname);
      break;
    }
    case USER_CERT: {
      // XXX switch  to using  other thing.
      scoped_refptr<X509Certificate> cert =
          x509_util::CreateX509CertificateFromCertCertificate(nss_cert);
      std::string subject_name = cert->subject().GetDisplayName();
      if (subject_name.empty()) {
        const char* email = CERT_GetFirstEmailAddress(nss_cert);
        if (email)
          subject_name = email;
      }
      // TODO(gspencer): Internationalize this. It's wrong to assume English
      // here.
      result = base::StringPrintf("%s's %s ID", subject_name.c_str(),
                                  cert->issuer().GetDisplayName().c_str());
      break;
    }
    case SERVER_CERT: {
      scoped_refptr<X509Certificate> cert =
          x509_util::CreateX509CertificateFromCertCertificate(nss_cert);
      result = cert->subject().GetDisplayName();
      break;
    }
    case OTHER_CERT:
    default:
      break;
  }
  return result;
}

}  // namespace

namespace x509_util {

bool IsSameCertificate(CERTCertificate* a, CERTCertificate* b) {
  DCHECK(a && b);
  if (a == b)
    return true;
  return a->derCert.len == b->derCert.len &&
         memcmp(a->derCert.data, b->derCert.data, a->derCert.len) == 0;
}

bool IsSameCertificate(CERTCertificate* a, const X509Certificate* b) {
#if BUILDFLAG(USE_BYTE_CERTS)
  return a->derCert.len == CRYPTO_BUFFER_len(b->os_cert_handle()) &&
         memcmp(a->derCert.data, CRYPTO_BUFFER_data(b->os_cert_handle()),
                a->derCert.len) == 0;
#else
  return IsSameCertificate(a, b->os_cert_handle());
#endif
}
bool IsSameCertificate(const X509Certificate* a, CERTCertificate* b) {
  return IsSameCertificate(b, a);
}

ScopedCERTCertificate CreateCERTCertificateFromBytes(const uint8_t* data,
                                                     size_t length) {
  return CreateCERTCertificateFromBytesWithNickname(data, length, nullptr);
}

// XXX convert other places using CERT_FindCertByDERCert to use this.
ScopedCERTCertificate FindCERTCertificateFromBytes(const uint8_t* data,
                                                   size_t length) {
  SECItem der_cert;
  der_cert.data = const_cast<uint8_t*>(data);
  der_cert.len = base::checked_cast<unsigned>(length);
  der_cert.type = siDERCertBuffer;
  ScopedCERTCertificate nss_cert(
      CERT_FindCertByDERCert(CERT_GetDefaultCertDB(), &der_cert));
  if (!nss_cert)
    return nullptr;

  if (nss_cert->derCert.len != length ||
      memcmp(nss_cert->derCert.data, data, length) != 0) {
    return nullptr;
  }

  return nss_cert;
}

ScopedCERTCertificate CreateCERTCertificateFromBytesWithNickname(
    const uint8_t* data,
    size_t length,
    const char* nickname) {
  crypto::EnsureNSSInit();

  if (!NSS_IsInitialized())
    return NULL;

  SECItem der_cert;
  der_cert.data = const_cast<uint8_t*>(data);
  der_cert.len = base::checked_cast<unsigned>(length);
  der_cert.type = siDERCertBuffer;

  // Parse into a certificate structure.
  return ScopedCERTCertificate(
      CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &der_cert,
                              const_cast<char*>(nickname), PR_FALSE, PR_TRUE));
}

ScopedCERTCertificate CreateCERTCertificateFromX509Certificate(
    const X509Certificate* cert) {
#if BUILDFLAG(USE_BYTE_CERTS)
  return CreateCERTCertificateFromBytes(
      CRYPTO_BUFFER_data(cert->os_cert_handle()),
      CRYPTO_BUFFER_len(cert->os_cert_handle()));
#else
  return DupCERTCertificate(cert->os_cert_handle());
// return ScopedCERTCertificate(CERT_DupCertificate(cert->os_cert_handle()));
#endif
}

ScopedCERTCertificateVector CreateCERTCertificateVectorFromX509Certificate(
    const X509Certificate* cert) {
#if BUILDFLAG(USE_BYTE_CERTS)
  ScopedCERTCertificateVector nss_chain;
  ScopedCERTCertificate nss_cert =
      CreateCERTCertificateFromX509Certificate(cert);
  if (!nss_cert)
    return ScopedCERTCertificateVector();
  nss_chain.push_back(std::move(nss_cert));
  for (net::X509Certificate::OSCertHandle intermediate :
       cert->GetIntermediateCertificates()) {
    ScopedCERTCertificate nss_intermediate = CreateCERTCertificateFromBytes(
        CRYPTO_BUFFER_data(intermediate), CRYPTO_BUFFER_len(intermediate));
    if (!nss_intermediate)
      return ScopedCERTCertificateVector();
    // XXX standardize push_back vs emplace_back
    // XXX use reserve in places?
    nss_chain.push_back(std::move(nss_intermediate));
  }
  return nss_chain;
#else
  ScopedCERTCertificateVector nss_chain;
  nss_chain.push_back(DupCERTCertificate(cert->os_cert_handle()));
  for (net::X509Certificate::OSCertHandle intermediate :
       cert->GetIntermediateCertificates()) {
    nss_chain.push_back(DupCERTCertificate(intermediate));
  }
  return nss_chain;
#endif
}

ScopedCERTCertificateVector CreateCertificateListFromBytes(const char* data,
                                                           size_t length,
                                                           int format) {
  CertificateList certs =
      X509Certificate::CreateCertificateListFromBytes(data, length, format);
  ScopedCERTCertificateVector nss_chain;
  for (const scoped_refptr<X509Certificate>& cert : certs) {
    ScopedCERTCertificate nss_cert =
        CreateCERTCertificateFromX509Certificate(cert.get());
    if (!nss_cert)
      return ScopedCERTCertificateVector();
    nss_chain.push_back(std::move(nss_cert));
  }
  return nss_chain;
}

ScopedCERTCertificate FindCERTCertificateFromX509Certificate(
    const X509Certificate* cert) {
#if BUILDFLAG(USE_BYTE_CERTS)
  return FindCERTCertificateFromBytes(
      CRYPTO_BUFFER_data(cert->os_cert_handle()),
      CRYPTO_BUFFER_len(cert->os_cert_handle()));
#else
  return DupCERTCertificate(cert->os_cert_handle());
// return ScopedCERTCertificate(CERT_DupCertificate(cert->os_cert_handle()));
#endif
}

ScopedCERTCertificate DupCERTCertificate(CERTCertificate* cert) {
  return ScopedCERTCertificate(CERT_DupCertificate(cert));
}

ScopedCERTCertificate DupCERTCertificate(const ScopedCERTCertificate& cert) {
  return DupCERTCertificate(cert.get());
}

ScopedCERTCertificateVector DupCERTCertificateVector(
    const ScopedCERTCertificateVector& certs) {
  ScopedCERTCertificateVector result;
  result.reserve(certs.size());
  for (const ScopedCERTCertificate& cert : certs)
    result.emplace_back(DupCERTCertificate(cert));
  return result;
}

scoped_refptr<X509Certificate> CreateX509CertificateFromCertCertificate(
    CERTCertificate* nss_cert,
    const std::vector<CERTCertificate*>& nss_chain) {
#if BUILDFLAG(USE_BYTE_CERTS)
  if (!nss_cert || !nss_cert->derCert.len)
    return nullptr;
  bssl::UniquePtr<CRYPTO_BUFFER> cert_handle(
      X509Certificate::CreateOSCertHandleFromBytes(
          reinterpret_cast<const char*>(nss_cert->derCert.data),
          nss_cert->derCert.len));
  if (!cert_handle)
    return nullptr;

  std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
  X509Certificate::OSCertHandles intermediates_raw;
  for (const CERTCertificate* nss_intermediate : nss_chain) {
    if (!nss_intermediate || !nss_intermediate->derCert.len)
      return nullptr;
    bssl::UniquePtr<CRYPTO_BUFFER> intermediate_cert_handle(
        X509Certificate::CreateOSCertHandleFromBytes(
            reinterpret_cast<const char*>(nss_intermediate->derCert.data),
            nss_intermediate->derCert.len));
    if (!intermediate_cert_handle)
      return nullptr;
    intermediates_raw.push_back(intermediate_cert_handle.get());
    intermediates.push_back(std::move(intermediate_cert_handle));
  }
  scoped_refptr<X509Certificate> result(
      X509Certificate::CreateFromHandle(cert_handle.get(), intermediates_raw));
  return result;
#else
  return X509Certificate::CreateFromHandle(nss_cert, nss_chain);
#endif
}

scoped_refptr<X509Certificate> CreateX509CertificateFromCertCertificate(
    CERTCertificate* cert) {
  return CreateX509CertificateFromCertCertificate(
      cert, std::vector<CERTCertificate*>());
}

CertificateList CreateX509CertificateListFromCERTCertificates(
    const ScopedCERTCertificateVector& certs) {
  CertificateList result;
  result.reserve(certs.size());
  for (const ScopedCERTCertificate& cert : certs) {
    scoped_refptr<X509Certificate> x509_cert(
        CreateX509CertificateFromCertCertificate(cert.get()));
    if (!x509_cert)
      return CertificateList();
    result.emplace_back(std::move(x509_cert));
  }
  return result;
}

bool GetDEREncoded(CERTCertificate* cert, std::string* der_encoded) {
  if (!cert || !cert->derCert.len)
    return false;
  der_encoded->assign(reinterpret_cast<char*>(cert->derCert.data),
                      cert->derCert.len);
  return true;
}

bool GetPEMEncoded(CERTCertificate* cert, std::string* pem_encoded) {
  if (!cert || !cert->derCert.len)
    return false;
  std::string der(reinterpret_cast<char*>(cert->derCert.data),
                  cert->derCert.len);
  return X509Certificate::GetPEMEncodedFromDER(der, pem_encoded);
}

void GetRFC822SubjectAltNames(CERTCertificate* cert_handle,
                              std::vector<std::string>* names) {
  crypto::ScopedSECItem alt_name(SECITEM_AllocItem(NULL, NULL, 0));
  DCHECK(alt_name.get());

  names->clear();
  SECStatus rv = CERT_FindCertExtension(
      cert_handle, SEC_OID_X509_SUBJECT_ALT_NAME, alt_name.get());
  if (rv != SECSuccess)
    return;

  crypto::ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  DCHECK(arena.get());

  CERTGeneralName* alt_name_list;
  alt_name_list = CERT_DecodeAltNameExtension(arena.get(), alt_name.get());

  CERTGeneralName* name = alt_name_list;
  while (name) {
    if (name->type == certRFC822Name) {
      names->push_back(
          std::string(reinterpret_cast<char*>(name->name.other.data),
                      name->name.other.len));
    }
    name = CERT_GetNextGeneralName(name);
    if (name == alt_name_list)
      break;
  }
}

void GetUPNSubjectAltNames(CERTCertificate* cert_handle,
                           std::vector<std::string>* names) {
  crypto::ScopedSECItem alt_name(SECITEM_AllocItem(NULL, NULL, 0));
  DCHECK(alt_name.get());

  names->clear();
  SECStatus rv = CERT_FindCertExtension(
      cert_handle, SEC_OID_X509_SUBJECT_ALT_NAME, alt_name.get());
  if (rv != SECSuccess)
    return;

  crypto::ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  DCHECK(arena.get());

  CERTGeneralName* alt_name_list;
  alt_name_list = CERT_DecodeAltNameExtension(arena.get(), alt_name.get());

  CERTGeneralName* name = alt_name_list;
  while (name) {
    if (name->type == certOtherName) {
      OtherName* on = &name->name.OthName;
      if (on->oid.len == sizeof(kUpnOid) &&
          memcmp(on->oid.data, kUpnOid, sizeof(kUpnOid)) == 0) {
        SECItem decoded;
        if (SEC_QuickDERDecodeItem(arena.get(), &decoded,
                                   SEC_ASN1_GET(SEC_UTF8StringTemplate),
                                   &name->name.OthName.name) == SECSuccess) {
          names->push_back(
              std::string(reinterpret_cast<char*>(decoded.data), decoded.len));
        }
      }
    }
    name = CERT_GetNextGeneralName(name);
    if (name == alt_name_list)
      break;
  }
}

std::string GetDefaultUniqueNickname(CERTCertificate* nss_cert,
                                     CertType type,
                                     PK11SlotInfo* slot) {
  return GetUniqueNicknameForSlot(GetDefaultNickname(nss_cert, type),
                                  &nss_cert->derSubject, slot);
}

bool GetValidityTimes(CERTCertificate* cert,
                      base::Time* not_before,
                      base::Time* not_after) {
  PRTime pr_not_before, pr_not_after;
  if (CERT_GetCertTimes(cert, &pr_not_before, &pr_not_after) == SECSuccess) {
    *not_before = crypto::PRTimeToBaseTime(pr_not_before);
    *not_after = crypto::PRTimeToBaseTime(pr_not_after);
    return true;
  }
  return false;
}

SHA256HashValue CalculateFingerprint256(CERTCertificate* cert) {
  SHA256HashValue sha256;
  memset(sha256.data, 0, sizeof(sha256.data));

  DCHECK(NULL != cert->derCert.data);
  DCHECK_NE(0U, cert->derCert.len);

  SECStatus rv = HASH_HashBuf(HASH_AlgSHA256, sha256.data, cert->derCert.data,
                              cert->derCert.len);
  DCHECK_EQ(SECSuccess, rv);

  return sha256;
}

}  // namespace x509_util

}  // namespace net
