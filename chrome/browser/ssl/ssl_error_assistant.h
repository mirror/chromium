// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_SSL_ERROR_ASSISTANT_H_
#define CHROME_BROWSER_SSL_SSL_ERROR_ASSISTANT_H_

#include <string>
#include <unordered_set>

#include "base/feature_list.h"
#include "chrome/browser/ssl/ssl_error_assistant.pb.h"
#include "chrome/common/features.h"
#include "net/ssl/ssl_info.h"

// Struct which stores data about a known MITM software pulled from the
// SSLErrorAssistant proto.
struct MITMSoftwareType {
  MITMSoftwareType(const std::string& name,
                   const std::string& issuer_common_name_regex,
                   const std::string& issuer_organization_regex)
      : name(name),
        issuer_common_name_regex(issuer_common_name_regex),
        issuer_organization_regex(issuer_organization_regex) {}

  const std::string name;
  const std::string issuer_common_name_regex;
  const std::string issuer_organization_regex;
};

// This class is responsible for deciding what type of interstitial to display
// for an SSL validation error and actually displaying it. The display of the
// interstitial might be delayed by a few seconds while trying to determine the
// cause of the error. During this window, the class will:
// - Check for a clock error
// - Check for a known captive portal certificate SPKI
// - Wait for a name-mismatch suggested URL
// - or Wait for a captive portal result to arrive.
// Based on the result of these checks, SSLErrorHandler will show a customized
// interstitial, redirect to a different suggested URL, or, if all else fails,
// show the normal SSL interstitial.
//
// This class should only be used on the UI thread because its implementation
// uses captive_portal::CaptivePortalService which can only be accessed on the
// UI thread.
class SSLErrorAssistant {
 public:
  SSLErrorAssistant();

  ~SSLErrorAssistant();

  // Returns true if any of the SHA256 hashes in |ssl_info| is of a captive
  // portal certificate. The set of captive portal hashes is loaded on first
  // use.
  bool ShouldDisplayCaptivePortalInterstitial(const net::SSLInfo& ssl_info,
                                              bool only_error_is_name_mismatch);

  // Returns the name of a known MITM software provider that matches the
  // certificate passed in as the |cert| parameter. Returns empty string if
  // there is no match.
  const std::string MatchKnownMITMSoftware(
      const scoped_refptr<net::X509Certificate> cert);

  void SetErrorAssistantProto(
      std::unique_ptr<chrome_browser_ssl::SSLErrorAssistantConfig> proto);

  // Testing methods:
  void ResetForTesting();

  int GetErrorAssistantProtoVersionIdForTesting() const;

 private:
  // SPKI hashes belonging to certs treated as captive portals. Null until the
  // first time IsKnownCaptivePortalCert() or SetErrorAssistantProto()
  // is called.
  std::unique_ptr<std::unordered_set<std::string>> captive_portal_spki_hashes_;

  std::unique_ptr<std::vector<MITMSoftwareType>> mitm_software_list_;

  // Error assistant configuration.
  std::unique_ptr<chrome_browser_ssl::SSLErrorAssistantConfig>
      error_assistant_proto_;

  DISALLOW_COPY_AND_ASSIGN(SSLErrorAssistant);
};

#endif  // CHROME_BROWSER_SSL_SSL_ERROR_ASSISTANT_H_
