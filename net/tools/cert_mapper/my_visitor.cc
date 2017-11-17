// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/my_visitor.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "net/cert/internal/cert_issuer_source_static.h"
#include "net/cert/internal/common_cert_errors.h"
#include "net/cert/internal/parsed_certificate.h"
#include "net/cert/internal/path_builder.h"
#include "net/cert/internal/signature_algorithm.h"
#include "net/cert/internal/simple_path_builder_delegate.h"
#include "net/cert/internal/system_trust_store.h"
#include "net/cert/internal/trust_store.h"
#include "net/cert/internal/trust_store_in_memory.h"
#include "net/cert/internal/verify_certificate_chain.h"
#include "net/der/encode_values.h"
#include "net/der/input.h"
#include "net/tools/cert_mapper/entry.h"
#include "net/tools/cert_mapper/metrics.h"
#include "net/tools/cert_mapper/visitor.h"

namespace net {

namespace {

// Parses |cert_data| and appends the parsed result to |chain|, and adds any
// parsing errors to |errors|.
bool AddParsedCertificate(const der::Input& cert_data,
                          ParsedCertificateList* chain,
                          CertErrors* errors) {
  ParseCertificateOptions options;
  options.allow_invalid_serial_numbers = false;  // TODO(eroman):
  scoped_refptr<ParsedCertificate> cert =
      ParsedCertificate::CreateWithoutCopyingUnsafe(
          cert_data.UnsafeData(), cert_data.Length(), options, errors);
  if (!cert)
    return false;
  chain->push_back(std::move(cert));
  return true;
}

void LogParsingCertErrors(const CertErrors& errors,
                          const std::string& name,
                          const scoped_refptr<CertBytes>& cert,
                          bool parsing_succeeded,
                          Metrics* metrics) {
  MetricsItem* item = metrics->GetAndIncrementTotal(name);

  // Identify all the errors.
  std::string errors_string;
  for (const auto& error_node : errors.nodes()) {
    errors_string += error_node.ToDebugString();
  }

  if (errors_string.empty() && !parsing_succeeded)
    errors_string = "Missing error details\n";

  item->GetAndIncrementTotal(errors_string)->AddSampleCert(cert);
}

bool GetFullChain(const Entry& entry,
                  ParsedCertificateList* chain,
                  Metrics* metrics) {
  chain->reserve(entry.certs.size());

  bool parsing_chain_succeeded = true;
  for (size_t i = 0; i < entry.certs.size(); ++i) {
    CertErrors errors;
    bool parsing_cert_succeeded =
        AddParsedCertificate(entry.certs[i]->AsInput(), chain, &errors);

    // Only log parsing failures once per leaf and once per intermediate.
    // Logging per chain would overcount intermediates which tend to be re-used
    // very often.
    if (i == 0) {
      const char* bucket_name =
          entry.type == EntryType::kUniqueIntermediateCertificate
              ? "Parse certificate errors (Intermediates)"
              : "Parse certificate errors (Leaf)";
      LogParsingCertErrors(errors, bucket_name, entry.certs[i],
                           parsing_cert_succeeded, metrics);
    }

    if (!parsing_cert_succeeded) {
      parsing_chain_succeeded = false;
    }
  }

  if (entry.type != EntryType::kUniqueIntermediateCertificate) {
    MetricsItem* item =
        metrics->GetAndIncrementTotal("Parse Certificate Chain");
    if (parsing_chain_succeeded) {
      item->GetAndIncrementTotal("Success");
    } else {
      item->GetAndIncrementTotal("Failure")->AddSampleChain(entry);
    }
  }

  return parsing_chain_succeeded;
}

base::Time GetTime(const der::GeneralizedTime& der_time) {
  base::Time::Exploded exploded;
  exploded.year = der_time.year;
  exploded.month = der_time.month;
  exploded.day_of_month = der_time.day;
  exploded.hour = der_time.hours;
  exploded.minute = der_time.minutes;
  exploded.second = der_time.seconds;
  exploded.millisecond = 0;

  base::Time t;
  if (!base::Time::FromUTCExploded(exploded, &t))
    return base::Time();
  return t;
}

bool CertUsesSha1(const scoped_refptr<ParsedCertificate>& cert) {
  return cert->signature_algorithm().digest() == DigestAlgorithm::Sha1;
}

const InitialExplicitPolicy kInitialExplicitPolicy =
    InitialExplicitPolicy::kFalse;
const InitialPolicyMappingInhibit kInitialPolicyMappingInhibit =
    InitialPolicyMappingInhibit::kFalse;
const InitialAnyPolicyInhibit kInitialAnyPolicyInhibit =
    InitialAnyPolicyInhibit::kFalse;

class MyVisitor : public Visitor {
 public:
  MyVisitor() : delegate_(1024) {
    now_time_ = base::Time::Now();
    CHECK(der::EncodeTimeAsGeneralizedTime(now_time_, &now_));
    user_initial_policy_set_ = {AnyPolicy()};
    system_trust_store_ = CreateSslSystemTrustStore();
  }

  void Visit(const Entry& entry, Metrics* metrics) override {
    // Count the entry types.
    MetricsItem* item = metrics->GetAndIncrementTotal("Entry types");
    switch (entry.type) {
      case EntryType::kCTCertificateChain:
        item->GetAndIncrementTotal("CT certificate chain");
        break;
      case EntryType::kUniqueIntermediateCertificate:
        item->GetAndIncrementTotal("Unique intermediate certificate");
        break;
      case EntryType::kTlsCertificateChain:
        item->GetAndIncrementTotal("TLS certificate chain");
        break;
    }

    // Parse all the certificates.
    ParsedCertificateList chain;
    if (!GetFullChain(entry, &chain, metrics))
      return;

    // For single certificates all we do is parse (skip chain validation).
    if (entry.type == EntryType::kUniqueIntermediateCertificate)
      return;

    // Verify the chain using an appropriate time. If the chain is expired, try
    // to pick a time during which it is not expired.
    base::Time not_before;
    base::Time not_after;

    GetChainExpiration(chain, &not_before, &not_after);
    bool chain_expired = now_time_ < not_before || now_time_ > not_after;

    if (chain_expired) {
      // Use the midpoint of the chain's validity.
      base::Time avg_t = not_before + (not_after - not_before) / 2;

      der::GeneralizedTime t;
      der::EncodeTimeAsGeneralizedTime(avg_t, &t);

      TestVerifyChain(chain, "[Expired] ", entry, t, metrics);
    } else {
      TestVerifyChain(chain, "", entry, now_, metrics);
    }
  }

 private:
  void TestVerifyChain(const ParsedCertificateList& chain,
                       const std::string& name_prefix,
                       const Entry& entry,
                       const der::GeneralizedTime& time,
                       Metrics* metrics) {
    switch (entry.type) {
      case EntryType::kTlsCertificateChain:
        // For certificate chains from TLS try to build a path to a publicly
        // trusted root and verify it using serverAuth.
        TestVerifyChainUsingPathBuilder(chain, name_prefix, entry, time,
                                        metrics,
                                        system_trust_store_->GetTrustStore());
        break;
      case EntryType::kCTCertificateChain:
        // For CT chains the entry already provides the full path including root
        // certificate. So no need to build a path, just verify the chain.
        TestVerifyChainUsingBuiltChain(chain, name_prefix, entry, time,
                                       metrics);
        break;
      case EntryType::kUniqueIntermediateCertificate:
        NOTREACHED();
        break;
    }
  }

  void TestVerifyChainUsingPathBuilder(const ParsedCertificateList& chain,
                                       const std::string& name_prefix,
                                       const Entry& entry,
                                       const der::GeneralizedTime& time,
                                       Metrics* metrics,
                                       TrustStore* trust_store) {
    // Add the intermediates.
    CertIssuerSourceStatic intermediates;
    for (size_t i = 1; i < chain.size(); ++i) {
      intermediates.AddCert(chain[i]);
    }

    KeyPurpose required_key_purpose = KeyPurpose::SERVER_AUTH;
    CertPathBuilder::Result result;
    CertPathBuilder path_builder(
        chain.front(), trust_store, &delegate_, time, required_key_purpose,
        kInitialExplicitPolicy, user_initial_policy_set_,
        kInitialPolicyMappingInhibit, kInitialAnyPolicyInhibit, &result);
    path_builder.AddCertIssuerSource(&intermediates);
    path_builder.Run();

    CertPathErrors* final_errors = nullptr;
    ParsedCertificateList* final_certs = nullptr;
    CertificateTrust final_trust;

    // Pick the best path (including failured one) from the result. If there
    // were no paths built synthesize an empty one.
    CertPathErrors no_path_errors;
    ParsedCertificateList empty_chain;

    if (!result.paths.empty()) {
      size_t index = result.best_result_index < result.paths.size()
                         ? result.best_result_index
                         : 0;
      final_errors = &result.paths[index]->errors;
      final_certs = &result.paths[index]->certs;
      final_trust = result.paths[index]->last_cert_trust;
    } else {
      final_errors = &no_path_errors;
      final_certs = &empty_chain;
      final_trust = CertificateTrust::ForUnspecified();

      final_errors->GetOtherErrors()->AddError(
          "Path building didn't find any paths");
    }

    if (!result.HasValidPath() && !final_errors->ContainsHighSeverityErrors()) {
      final_errors->GetOtherErrors()->AddError(
          "UNEXPECTED - no errors set for path");
    }

    LogVerifyChainResult(*final_certs, final_trust, final_errors, name_prefix,
                         entry, metrics);
  }

  void TestVerifyChainUsingBuiltChain(const ParsedCertificateList& chain,
                                      const std::string& name_prefix,
                                      const Entry& entry,
                                      const der::GeneralizedTime& time,
                                      Metrics* metrics) {
    CertificateTrust last_cert_trust = CertificateTrust::ForTrustAnchor();
    CertPathErrors chain_errors;
    std::set<der::Input> user_constrained_policy_set;
    VerifyCertificateChain(
        chain, last_cert_trust, &delegate_, time, KeyPurpose::SERVER_AUTH,
        kInitialExplicitPolicy, user_initial_policy_set_,
        kInitialPolicyMappingInhibit, kInitialAnyPolicyInhibit,
        &user_constrained_policy_set, &chain_errors);

    LogVerifyChainResult(chain, last_cert_trust, &chain_errors, name_prefix,
                         entry, metrics);
  }

  void LogVerifyChainResult(const ParsedCertificateList& chain,
                            const CertificateTrust& last_certificate_trust,
                            CertPathErrors* chain_errors,
                            const std::string& name_prefix,
                            const Entry& entry,
                            Metrics* metrics) {
    size_t end_intermediates = chain.size();
    if (last_certificate_trust.IsTrustAnchor())
      end_intermediates--;

    // Add errors to the chain if SHA1 was used. Break it down by intermediate
    // vs target (and don't count root certificates).
    for (size_t i = 0; i < chain.size(); ++i) {
      if (CertUsesSha1(chain[i])) {
        if (i == 0) {
          chain_errors->GetOtherErrors()->AddWarning("Chain uses SHA1 leaf");
        } else if (i < end_intermediates) {
          chain_errors->GetOtherErrors()->AddWarning(
              "Chain uses SHA1 intermediate");
        }
      }
    }

    // Log whether validation succeeded.
    {
      MetricsItem* item =
          metrics->GetAndIncrementTotal(name_prefix + "Verify Chain");
      if (chain_errors->ContainsHighSeverityErrors()) {
        item->GetAndIncrementTotal("Failure");
      } else {
        item->GetAndIncrementTotal("Success");
      }
    }

    // Log the length of the input chain.
    {
      MetricsItem* item = metrics->GetAndIncrementTotal("Input chain length");
      item->GetAndIncrementTotal(base::SizeTToString(entry.certs.size()))
          ->AddSampleChain(entry);
    }

    // Log the breakdown of the errors.
    {
      MetricsItem* item =
          metrics->GetAndIncrementTotal(name_prefix + "Verify Chain Errors");

      // Identify all the errors.
      std::set<std::string> error_set;
      for (size_t i = 0; i < chain_errors->Count(); ++i) {
        const CertErrors* cert_errors = chain_errors->GetErrorsForCert(i);
        if (cert_errors) {
          for (const auto& error_node : cert_errors->nodes()) {
            error_set.insert(error_node.ToDebugString());
          }
        }
      }

      if (chain_errors->GetOtherErrors()) {
        for (const auto& error_node : chain_errors->GetOtherErrors()->nodes()) {
          error_set.insert(error_node.ToDebugString());
        }
      }

      for (const auto& error : error_set) {
        item->GetAndIncrementTotal(error)->AddSampleChain(entry);
      }
    }
  }

  void GetChainExpiration(const ParsedCertificateList& chain,
                          base::Time* not_before,
                          base::Time* not_after) const {
    *not_before = base::Time();
    *not_after = base::Time::Max();

    // Determine whether any certificate in the chain (except root) is expired.
    for (size_t i = 0; i + 1 < chain.size(); ++i) {
      const auto& tbs = chain[i]->tbs();
      *not_before = std::max(GetTime(tbs.validity_not_before), *not_before);
      *not_after = std::min(GetTime(tbs.validity_not_after), *not_after);
    }
  }

  der::GeneralizedTime now_;
  base::Time now_time_;
  std::set<der::Input> user_initial_policy_set_;
  SimplePathBuilderDelegate delegate_;

  std::unique_ptr<SystemTrustStore> system_trust_store_;
};

class MyVisitorFactory : public VisitorFactory {
 public:
  std::unique_ptr<Visitor> Create() override {
    return base::MakeUnique<MyVisitor>();
  }
};

}  // namespace

std::unique_ptr<VisitorFactory> CreateMyVisitorFactory() {
  return base::MakeUnique<MyVisitorFactory>();
}

}  // namespace net
