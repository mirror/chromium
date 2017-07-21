// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_X509_UTIL_NSS_H_
#define NET_CERT_X509_UTIL_NSS_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "net/base/net_export.h"
#include "net/cert/cert_type.h"
#include "net/cert/scoped_nss_types.h"
#include "net/cert/x509_certificate.h"

typedef struct CERTCertificateStr CERTCertificate;
typedef struct PK11SlotInfoStr PK11SlotInfo;
typedef struct SECItemStr SECItem;

namespace net {

namespace x509_util {

// XXX standardize naming (CertCertificate vs CERTCertificate, etc)

// XXX docs
NET_EXPORT bool IsSameCertificate(CERTCertificate* a, CERTCertificate* b);

NET_EXPORT bool IsSameCertificate(CERTCertificate* a, const X509Certificate* b);
NET_EXPORT bool IsSameCertificate(const X509Certificate* a, CERTCertificate* b);

// XXX make sure everything calling x509_util::foo error checks the results
// XXX are all places these CreateCERTCertificateFromXXX used okay with NULL
// nickname?
//
// XXX docs
NET_EXPORT ScopedCERTCertificate
CreateCERTCertificateFromBytes(const uint8_t* data, size_t length);
NET_EXPORT ScopedCERTCertificate
CreateCERTCertificateFromBytesWithNickname(const uint8_t* data,
                                           size_t length,
                                           const char* nickname);

// Similar to CreateCERTCertificateFromBytes, but the certificate must already
// exist in NSS (possibly as a temp certificate).
NET_EXPORT ScopedCERTCertificate
FindCERTCertificateFromBytes(const uint8_t* data, size_t length);

// XXX docs
NET_EXPORT ScopedCERTCertificate
CreateCERTCertificateFromX509Certificate(const X509Certificate* cert);
NET_EXPORT ScopedCERTCertificateVector
CreateCERTCertificateVectorFromX509Certificate(const X509Certificate* cert);

// XXX rename? and docs
NET_EXPORT ScopedCERTCertificateVector
CreateCertificateListFromBytes(const char* data, size_t length, int format);

// Similar to CreateCERTCertificateFromX509Certificate, but the certificate
// must already exist in NSS (possibly as a temp certificate).
NET_EXPORT ScopedCERTCertificate
FindCERTCertificateFromX509Certificate(const X509Certificate* cert);

// XXX docs
// XXX replace direct usages of CERT_DupCertificate
NET_EXPORT ScopedCERTCertificate
DupCERTCertificate(const ScopedCERTCertificate& cert);
NET_EXPORT ScopedCERTCertificate DupCERTCertificate(CERTCertificate* cert);
NET_EXPORT ScopedCERTCertificateVector
DupCERTCertificateVector(const ScopedCERTCertificateVector& certs);

// XXX docs
NET_EXPORT scoped_refptr<X509Certificate>
CreateX509CertificateFromCertCertificate(
    CERTCertificate* cert,
    const std::vector<CERTCertificate*>& chain);
NET_EXPORT scoped_refptr<X509Certificate>
CreateX509CertificateFromCertCertificate(CERTCertificate* cert);

NET_EXPORT CertificateList CreateX509CertificateListFromCERTCertificates(
    const ScopedCERTCertificateVector& certs);

NET_EXPORT bool GetDEREncoded(CERTCertificate* cert, std::string* der_encoded);
NET_EXPORT bool GetPEMEncoded(CERTCertificate* cert, std::string* pem_encoded);

// Stores the values of all rfc822Name subjectAltNames from |cert_handle|
// into |names|. If no names are present, clears |names|.
// WARNING: This method does not validate that the rfc822Name is
// properly encoded; it MAY contain embedded NULs or other illegal
// characters; care should be taken to validate the well-formedness
// before using.
NET_EXPORT void GetRFC822SubjectAltNames(CERTCertificate* cert_handle,
                                         std::vector<std::string>* names);

// Stores the values of all Microsoft UPN subjectAltNames from |cert_handle|
// into |names|. If no names are present, clears |names|.
//
// A "Microsoft UPN subjectAltName" is an OtherName value whose type-id
// is equal to 1.3.6.1.4.1.311.20.2.3 (known as either id-ms-san-sc-logon-upn,
// as described in RFC 4556, or as szOID_NT_PRINCIPAL_NAME, as
// documented in Microsoft KB287547).
// The value field is a UTF8String literal.
// For more information:
//   https://www.ietf.org/mail-archive/web/pkix/current/msg03145.html
//   https://www.ietf.org/proceedings/65/slides/pkix-4/sld1.htm
//   https://tools.ietf.org/html/rfc4556
//
// WARNING: This method does not validate that the name is
// properly encoded; it MAY contain embedded NULs or other illegal
// characters; care should be taken to validate the well-formedness
// before using.
NET_EXPORT void GetUPNSubjectAltNames(CERTCertificate* cert_handle,
                                      std::vector<std::string>* names);

// Generates a unique nickname for |nss_cert| based on the |type| and |slot|.
NET_EXPORT std::string GetDefaultUniqueNickname(CERTCertificate* nss_cert,
                                                CertType type,
                                                PK11SlotInfo* slot);

// XXX
NET_EXPORT bool GetValidityTimes(CERTCertificate* cert,
                                 base::Time* not_before,
                                 base::Time* not_after);

// XXX
NET_EXPORT SHA256HashValue CalculateFingerprint256(CERTCertificate* cert);

} // namespace x509_util

} // namespace net

#endif  // NET_CERT_X509_UTIL_NSS_H_
