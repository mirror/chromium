// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_SSL_ERROR_ASSISTANT_H_
#define CHROME_BROWSER_SSL_SSL_ERROR_ASSISTANT_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "net/ssl/ssl_info.h"
#include "url/gurl.h"

namespace chrome_browser_ssl {
class SSLErrorAssistantConfig;
class UrgentInterstitial;
}  // namespace chrome_browser_ssl

namespace net {
class SSLInfo;
}

// Enum class used to represent the interstitial page that would be displayed
// for an urgent interstitial.
enum class UrgentInterstitialPageType {
  NONE = 0,
  SSL,
  CAPTIVE_PORTAL,
  MITM_SOFTWARE
};

// Struct which stores data about a known MITM software pulled from the
// SSLErrorAssistant proto.
struct MITMSoftwareType {
  MITMSoftwareType(const std::string& name,
                   const std::string& issuer_common_name_regex,
                   const std::string& issuer_organization_regex);

  const std::string name;
  const std::string issuer_common_name_regex;
  const std::string issuer_organization_regex;
};

class UrgentInterstitial {
 public:
  explicit UrgentInterstitial(
      const chrome_browser_ssl::UrgentInterstitial& entry);
  ~UrgentInterstitial();

  // Returns an UrgentInterstitial object if any of the SHA256 hashes in
  // |ssl_info| matches the hashes in |spki_hashes|.
  bool MatchCertificate(const net::SSLInfo& ssl_info);

  UrgentInterstitialPageType interstitial_type() const {
    return interstitial_type_;
  }

  GURL support_url() const { return support_url_; }

 private:
  std::unordered_set<std::string> spki_hashes_;
  const int error_code_;
  const UrgentInterstitialPageType interstitial_type_;
  const GURL support_url_;

  DISALLOW_COPY_AND_ASSIGN(UrgentInterstitial);
};

// Helper class for SSLErrorHandler. This class is responsible for reading in
// the ssl_error_assistant protobuf and parsing through the data.
class SSLErrorAssistant {
 public:
  SSLErrorAssistant();

  ~SSLErrorAssistant();

  // Returns true if any of the SHA256 hashes in |ssl_info| is of a captive
  // portal certificate. The set of captive portal hashes is loaded on first
  // use.
  bool IsKnownCaptivePortalCertificate(const net::SSLInfo& ssl_info);

  // Returns the name of a known MITM software provider that matches the
  // certificate passed in as the |cert| parameter. Returns empty string if
  // there is no match.
  const std::string MatchKnownMITMSoftware(
      const scoped_refptr<net::X509Certificate>& cert);

  // Returns an UrgentInterstitial object if any of the SHA256 hashes in
  // |ssl_info| matches the hashes in any of UrgentInterstitials. The set of
  // UrgentInterstials is loaded on the first use. Returns null if there is
  // no match.
  UrgentInterstitial* MatchUrgentInterstitial(const net::SSLInfo& ssl_info);

  void SetErrorAssistantProto(
      std::unique_ptr<chrome_browser_ssl::SSLErrorAssistantConfig> proto);

  // Testing methods:
  void ResetForTesting();
  int GetErrorAssistantProtoVersionIdForTesting() const;

 private:
  // SPKI hashes belonging to certs treated as captive portals. Null until the
  // first time ShouldDisplayCaptiveProtalInterstitial() or
  // SetErrorAssistantProto() is called.
  std::unique_ptr<std::unordered_set<std::string>> captive_portal_spki_hashes_;

  // Data about a known MITM software pulled from the SSLErrorAssistant proto.
  // Null until MatchKnownMITMSoftware() is called.
  std::unique_ptr<std::vector<MITMSoftwareType>> mitm_software_list_;

  // Data about the urgent interstitials pulled from the SSLErrorAssistant
  // proto. Null until MatchUrgentInterstitial() is called.
  std::vector<std::unique_ptr<UrgentInterstitial>> urgent_interstitial_list_;

  // Error assistant configuration.
  std::unique_ptr<chrome_browser_ssl::SSLErrorAssistantConfig>
      error_assistant_proto_;

  DISALLOW_COPY_AND_ASSIGN(SSLErrorAssistant);
};

#endif  // CHROME_BROWSER_SSL_SSL_ERROR_ASSISTANT_H_
