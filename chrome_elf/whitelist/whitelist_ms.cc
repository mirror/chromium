// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_ms.h"

#include <windows.h>

#include <wincrypt.h>  // crypt32.dll
#include <wintrust.h>  // wintrust.dll
// This must be after wincrypt and wintrust.
#include <mscat.h>  // wintrust.dll

#include <string>
#include <vector>

#include <assert.h>

#include "sandbox/win/src/nt_internals.h"

namespace {

//------------------------------------------------------------------------------
// Private defines
//------------------------------------------------------------------------------

// The type of certificate found for the module.
enum CertificateType {
  // The module is not signed.
  NO_CERTIFICATE = 0,
  // The module is signed and the certificate is in the module.
  CERTIFICATE_IN_FILE,
  // The module is signed and the certificate is in an external catalog.
  CERTIFICATE_IN_CATALOG,
};

struct CertificateInfo {
  CertificateInfo() : type(NO_CERTIFICATE) {}

  // The type of signature encountered.
  CertificateType type;

  // Path to the file containing the certificate. Empty if |type| is
  // NO_CERTIFICATE.
  std::wstring path;

  // The "Subject" name of the certificate. This is the signer (ie,
  // "Google Inc." or "Microsoft Inc.").
  std::wstring subject;
};

//------------------------------------------------------------------------------
// Private functions
//------------------------------------------------------------------------------

bool GetMsgSignerInfo(const HCRYPTMSG msg, std::vector<BYTE>* info_buffer) {
  assert(msg);
  assert(info_buffer);

  // Determine the size of the signer info data.
  // Note: only grabbing signer at index 0 right now.  Could be more than one
  //       signer in a message.
  DWORD signer_info_size = 0;
  if (!::CryptMsgGetParam(msg, CMSG_SIGNER_INFO_PARAM, 0, nullptr,
                          &signer_info_size)) {
    return false;
  }

  // Obtain the signer info.
  // Note: only grabbing signer at index 0 right now.  Could be more than one
  //       signer in a message.
  info_buffer->resize(signer_info_size);
  if (!::CryptMsgGetParam(msg, CMSG_SIGNER_INFO_PARAM, 0, info_buffer->data(),
                          &signer_info_size)) {
    return false;
  }

  return true;
}

bool GetSubjectFromCert(const PCCERT_CONTEXT cert_context, std::wstring* name) {
  assert(cert_context);
  assert(name);

  // Determine the size of the subject name.
  // Note: always return size of at least 1, for EoS.
  DWORD subject_name_size = ::CertGetNameStringW(
      cert_context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, nullptr, 0);
  if (subject_name_size <= 1)
    return false;

  // Get subject name.
  name->resize(subject_name_size);
  subject_name_size =
      ::CertGetNameStringW(cert_context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,
                           nullptr, &(*name)[0], subject_name_size);
  if (subject_name_size <= 1)
    return false;

  return true;
}

// Harvest the subject name from an embedded certificate in the given file, if
// there is one.
//
// NOTE:
//   - MS_SUCCESS: no failure, embedded cert info found.
//   - MS_NO_EMBEDDED_CERT: no failure, just no embedded cert.
whitelist::MsStatus GetSubjectNameInFile(const wchar_t* path,
                                         std::wstring* subject) {
  assert(path);
  assert(subject);

  // Find the crypto message for this filename.
  //
  // - The object is stored in a file.
  // - The content is an embedded PKCS #7 signed message.
  // - The content should be returned in binary format.
  // - |cert_store| receives a handle to a store that includes all of the
  //   certificates in the target file.
  // - |crypt_message| receives a handle to the opened crypt message.
  HCERTSTORE cert_store = nullptr;
  HCRYPTMSG crypt_message = nullptr;
  if (!::CryptQueryObject(CERT_QUERY_OBJECT_FILE, path,
                          CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                          CERT_QUERY_FORMAT_FLAG_BINARY, 0, nullptr, nullptr,
                          nullptr, &cert_store, &crypt_message, nullptr)) {
    // TODO(pennymac): run some tests, GetLastError() and see what system error
    //                 codes are commonly returned.  Fine tune this to catch
    //                 specific case of no embedded cert.
    return whitelist::MS_NO_EMBEDDED_CERT;
  }

  // Get signer info from crypt_message.
  std::vector<BYTE> signer_info_buffer;
  if (!GetMsgSignerInfo(crypt_message, &signer_info_buffer)) {
    ::CertCloseStore(cert_store, 0);
    ::CryptMsgClose(crypt_message);
    return whitelist::MS_GET_SIGNER_INFO_FAIL;
  }
  CMSG_SIGNER_INFO* signer_info =
      reinterpret_cast<CMSG_SIGNER_INFO*>(signer_info_buffer.data());

  // Release the crypt_message when done with it.
  ::CryptMsgClose(crypt_message);

  // Search for the signer certificate.
  //
  // - CERT_FIND_SUBJECT_CERT: Searches for a certificate with both an issuer
  // and a serial number that match the issuer and serial number in the
  // CERT_INFO structure.
  CERT_INFO cert_info = {0};
  cert_info.Issuer = signer_info->Issuer;
  cert_info.SerialNumber = signer_info->SerialNumber;

  PCCERT_CONTEXT cert_context = ::CertFindCertificateInStore(
      cert_store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0,
      CERT_FIND_SUBJECT_CERT, &cert_info, nullptr);

  // Release the cert_store when done with it.
  ::CertCloseStore(cert_store, 0);

  if (!cert_context)
    return whitelist::MS_GET_CERT_FOR_SIGNER_FAIL;

  if (!GetSubjectFromCert(cert_context, subject)) {
    ::CertFreeCertificateContext(cert_context);
    return whitelist::MS_GET_SUBJECT_FROM_CERT_FAIL;
  }

  // Release the cert_context when done with it.
  ::CertFreeCertificateContext(cert_context);

  return whitelist::MS_SUCCESS;
}

bool GetFileHash(const std::wstring& path, std::vector<BYTE>* hash) {
  assert(hash);

  // Open the file.  INVALID_HANDLE_VALUE alert.
  HANDLE file =
      ::CreateFileW(path.c_str(), GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, 0, nullptr);
  if (file == INVALID_HANDLE_VALUE)
    return false;

  // Get the size we need for our hash.
  DWORD hash_size = 0;
  ::CryptCATAdminCalcHashFromFileHandle(file, &hash_size, nullptr, 0);
  if (hash_size == 0) {
    assert(false);
    ::CloseHandle(file);
    return false;
  }

  // Calculate the hash. If this fails then bail.
  hash->resize(hash_size);
  if (!::CryptCATAdminCalcHashFromFileHandle(file, &hash_size, hash->data(),
                                             0)) {
    assert(false);
    ::CloseHandle(file);
    return false;
  }

  // Done with the file handle.
  ::CloseHandle(file);

  return true;
}

// Harvest catalog certificate information, if any found.
//
// NOTE:
//   - MS_SUCCESS: no failure, catalog cert info found.
//   - MS_NO_CATALOG: no failure, just no associated catalog.
whitelist::MsStatus GetCatalogCertificateInfo(
    const std::wstring& path,
    CertificateInfo* certificate_info) {
  std::vector<BYTE> hash;
  // Get a hash of the file.
  if (!GetFileHash(path, &hash))
    return whitelist::MS_FILE_HASH_FAIL;

  // Get a crypt admin context for signature verification.
  PVOID admin_context = nullptr;
  if (!::CryptCATAdminAcquireContext(&admin_context, nullptr, 0))
    return whitelist::MS_GET_ADMIN_CONTEXT_FAIL;

  // Get the first catalog that contains this hash.
  PVOID catalog_context = ::CryptCATAdminEnumCatalogFromHash(
      admin_context, hash.data(), hash.size(), 0, nullptr);

  // Release the admin context.
  ::CryptCATAdminReleaseContext(admin_context, NULL);

  // There might not be a catalog for this hash.
  if (!catalog_context)
    return whitelist::MS_NO_CATALOG;

  // Get the catalog info. This includes the path to the catalog itself, which
  // contains the signature of interest.
  CATALOG_INFO catalog_info = {};
  catalog_info.cbStruct = sizeof(catalog_info);
  if (!::CryptCATCatalogInfoFromContext(catalog_context, &catalog_info, 0))
    return whitelist::MS_GET_CATALOG_INFO_FROM_CONTEXT_FAIL;

  // Attempt to get the "Subject" field from the signature of the catalog file
  // itself.
  if (!GetSubjectNameInFile(catalog_info.wszCatalogFile,
                            &certificate_info->subject)) {
    return whitelist::MS_GET_SUBJECT_FROM_CATALOG_FAIL;
  }
  certificate_info->type = CertificateType::CERTIFICATE_IN_CATALOG;
  certificate_info->path = catalog_info.wszCatalogFile;

  return whitelist::MS_SUCCESS;
}

// Extracts information about the certificate of the given PE file, if any is
// found.
//
// NOTE:
//   - MS_SUCCESS: no failure, cert info found.
//   - MS_FILE_NOT_SIGNED: no failure, just no catalog or embedded signature!
whitelist::MsStatus GetCertificateInfo(const std::wstring& path,
                                       CertificateInfo* certificate_info) {
  assert(certificate_info->type == CertificateType::NO_CERTIFICATE);
  assert(certificate_info->path.empty());
  assert(certificate_info->subject.empty());

  // 1. Check for a catalog cert.
  whitelist::MsStatus status =
      GetCatalogCertificateInfo(path, certificate_info);
  if (status == whitelist::MS_SUCCESS) {
    // All done.
    assert(certificate_info->type == CertificateType::CERTIFICATE_IN_CATALOG);
    return status;
  }
  if (status != whitelist::MS_NO_CATALOG) {
    // Unexpected failure.
    return status;
  }

  // 2. Check for an embedded cert.
  status = GetSubjectNameInFile(path.c_str(), &certificate_info->subject);
  if (status == whitelist::MS_SUCCESS) {
    // All done.
    certificate_info->type = CertificateType::CERTIFICATE_IN_FILE;
    certificate_info->path = path;
    return status;
  }
  if (status != whitelist::MS_NO_EMBEDDED_CERT) {
    // Unexpected failure.
    return status;
  }

  return whitelist::MS_FILE_NOT_SIGNED;
}

}  // namespace

namespace whitelist {

//------------------------------------------------------------------------------
// Public
//------------------------------------------------------------------------------

MsStatus CheckImageSignature(const std::wstring& path) {
  // If signer subject name starts with this, whitelist.
  static constexpr wchar_t kMicrosoft[] = L"Microsoft ";
  // If signer subject name is exactly this, whitelist.
  static constexpr wchar_t kGoogle[] = L"Google Inc";

  // 1. Get any certificate information for the given image.
  CertificateInfo cert_info;
  MsStatus status = GetCertificateInfo(path, &cert_info);
  if (status == MS_FILE_NOT_SIGNED || status != MS_SUCCESS)
    return status;

  // 2. Check signer subject name.
  if (cert_info.subject == kGoogle ||
      cert_info.subject.compare(0, ::wcslen(kMicrosoft), kMicrosoft) == 0)
    return MS_SUCCESS;

  return MS_BLOCK_BINARY;
}

}  // namespace whitelist
