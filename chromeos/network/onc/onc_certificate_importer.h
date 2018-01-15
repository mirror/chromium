// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_ONC_ONC_CERTIFICATE_IMPORTER_H_
#define CHROMEOS_NETWORK_ONC_ONC_CERTIFICATE_IMPORTER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromeos/chromeos_export.h"
#include "components/onc/onc_constants.h"
#include "net/cert/scoped_nss_types.h"

namespace base {
class ListValue;
class Value;
}  // namespace base

namespace net {
class NSSCertDatabase;
}

namespace chromeos {
namespace onc {

class CHROMEOS_EXPORT CertificateImporter {
 public:
  class Factory {
   public:
    // Result callback for GetTrustedCertificatesStatus.
    // |available_trusted_certs| will be filled with certificates which
    // requested web trust and are already permanently available in a NSS
    // database.  |has_missing_trusted_certs| will be set to true if any
    // certificate that requested web trust is not permantely avilable in a NSS
    // database.
    using GotTrustedCertificatesStatusCallback = base::OnceCallback<void(
        net::ScopedCERTCertificateList available_trusted_certs,
        bool has_missing_trusted_certs)>;

    virtual ~Factory() {}

    // Creates a CertificateImporter for the specified |database|.
    virtual std::unique_ptr<chromeos::onc::CertificateImporter>
    CreateCertificateImporter(net::NSSCertDatabase* database) = 0;

    // Determines the status of server or authority certificates which request
    // web trust.  This is in the CertificateImporter::Factory because it does
    // not depend on a certificate database, and should be callable before a
    // user's certificate database (which references the user's private slot) is
    // available.
    virtual void GetTrustedCertificatesStatus(
        base::Value onc_certificates,
        ::onc::ONCSource source,
        GotTrustedCertificatesStatusCallback callback) = 0;
  };

  typedef base::Callback<void(
      bool success,
      net::ScopedCERTCertificateList onc_trusted_certificates)>
      DoneCallback;

  CertificateImporter() {}
  virtual ~CertificateImporter() {}

  // Import |certificates|, which must be a list of ONC Certificate objects.
  // Certificates are only imported with web trust for user imports. If the
  // "Remove" field of a certificate is enabled, then removes the certificate
  // from the store instead of importing.
  // When the import is completed, |done_callback| will be called with |success|
  // equal to true if all certificates were imported successfully.
  // |onc_trusted_certificates| will contain the list of certificates that
  // were imported and requested the TrustBit "Web".
  // Never calls |done_callback| after this importer is destructed.
  virtual void ImportCertificates(const base::ListValue& certificates,
                                  ::onc::ONCSource source,
                                  const DoneCallback& done_callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CertificateImporter);
};

}  // namespace onc
}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_ONC_ONC_CERTIFICATE_IMPORTER_H_
