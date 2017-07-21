// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/certificate_helper.h"

#include <cert.h>
#include <certdb.h>
#include <pk11pub.h>
#include <secport.h>

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "components/url_formatter/url_formatter.h"
#include "net/cert/nss_cert_database_chromeos.h"

namespace chromeos {
namespace certificate {

namespace {

// Convert a char* return value from NSS into a std::string and free the NSS
// memory. If |nss_text| is null, |alternative_text| will be returned instead.
std::string Stringize(char* nss_text, const std::string& alternative_text) {
  if (!nss_text)
    return alternative_text;

  std::string s = nss_text;
  PORT_Free(nss_text);
  return !s.empty() ? s : alternative_text;
}

std::string GetNickname(CERTCertificate* cert_handle) {
  if (!cert_handle->nickname)
    return std::string();
  std::string name = cert_handle->nickname;
  // Hack copied from mozilla: Cut off text before first :, which seems to
  // just be the token name.
  size_t colon_pos = name.find(':');
  if (colon_pos != std::string::npos)
    name = name.substr(colon_pos + 1);
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

}  // namespace

net::CertType GetCertType(CERTCertificate* cert_handle) {
  CERTCertTrust trust = {0};
  CERT_GetCertTrust(cert_handle, &trust);

  unsigned all_flags =
      trust.sslFlags | trust.emailFlags | trust.objectSigningFlags;

  if (cert_handle->nickname && (all_flags & CERTDB_USER))
    return net::USER_CERT;

  if ((all_flags & CERTDB_VALID_CA) || CERT_IsCACert(cert_handle, nullptr))
    return net::CA_CERT;

  // TODO(mattm): http://crbug.com/128633.
  if (trust.sslFlags & CERTDB_TERMINAL_RECORD)
    return net::SERVER_CERT;

  return net::OTHER_CERT;
}

std::string GetCertTokenName(CERTCertificate* cert_handle) {
  std::string token;
  if (cert_handle->slot)
    token = PK11_GetTokenName(cert_handle->slot);
  return token;
}

std::string GetIssuerDisplayName(CERTCertificate* cert_handle) {
  return GetDisplayName(&cert_handle->issuer);
}

std::string GetCertNameOrNickname(CERTCertificate* cert_handle) {
  std::string name = GetCertAsciiNameOrNickname(cert_handle);
  if (!name.empty())
    name = base::UTF16ToUTF8(url_formatter::IDNToUnicode(name));
  return name;
}

std::string GetCertAsciiNameOrNickname(CERTCertificate* cert_handle) {
  std::string alternative_text = GetNickname(cert_handle);
  return Stringize(CERT_GetCommonName(&cert_handle->subject), alternative_text);
}

}  // namespace certificate
}  // namespace chromeos
