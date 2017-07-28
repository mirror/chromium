// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/my_visitor.h"

#include <algorithm>

#include "base/strings/stringprintf.h"
#include "net/cert/internal/parsed_certificate.h"
#include "net/cert/internal/simple_path_builder_delegate.h"
#include "net/cert/internal/trust_store.h"
#include "net/cert/internal/verify_certificate_chain.h"
#include "net/der/encode_values.h"
#include "net/der/input.h"
#include "net/tools/cert_mapper/entry.h"
#include "net/tools/cert_mapper/metrics.h"
#include "net/tools/cert_mapper/visitor.h"

namespace net {

namespace {

bool AddParsedCertificate(const der::Input& cert_data,
                          ParsedCertificateList* chain,
                          CertErrors* errors) {
  ParseCertificateOptions options;
  options.allow_invalid_serial_numbers = true;
  scoped_refptr<ParsedCertificate> cert =
      ParsedCertificate::CreateWithoutCopyingUnsafe(
          cert_data.UnsafeData(), cert_data.Length(), options, errors);
  if (!cert) {
    return false;
  }
  chain->push_back(std::move(cert));
  return true;
}

void LogCertPathErrors(CertPathErrors* errors,
                       const ParsedCertificateList& chain,
                       const std::string& name,
                       const Entry& entry,
                       CertError::Severity severity,
                       Metrics* metrics) {
  if (errors->ContainsAnyErrorWithSeverity(severity)) {
    MetricsItem* item = metrics->GetAndIncrementTotal(name);

    // Identify all the errors.
    std::set<std::string> error_set;
    for (size_t i = 0; i < chain.size(); ++i) {
      CertErrors* cert_errors = errors->GetErrorsForCert(i);
      for (const auto& error_node : cert_errors->nodes()) {
        if (error_node.severity == severity) {
          error_set.insert(error_node.ToDebugString());
        }
      }
    }

    for (const auto& error : error_set) {
      item->GetAndIncrementTotal(error)->AddSampleChain(entry);
    }
  }
}

void LogCertErrors(const CertErrors& errors,
                   const std::string& name,
                   const scoped_refptr<CertBytes>& cert,
                   CertError::Severity severity,
                   bool parsing_succeeded,
                   Metrics* metrics) {
  if (errors.ContainsAnyErrorWithSeverity(severity)) {
    MetricsItem* item = metrics->GetAndIncrementTotal(name);

    // Identify all the errors.
    std::set<std::string> error_set;
    for (const auto& error_node : errors.nodes()) {
      if (error_node.severity == severity) {
        error_set.insert(error_node.ToDebugString());
      }
    }

    for (const auto& error : error_set) {
      item->GetAndIncrementTotal(error)->AddSampleCert(cert);
    }
  } else if (!parsing_succeeded && severity == CertError::SEVERITY_HIGH) {
    metrics->GetAndIncrementTotal(name)
        ->GetAndIncrementTotal("Unknown")
        ->AddSampleCert(cert);
  }
}

bool GetFullChain(const Entry& entry,
                  ParsedCertificateList* chain,
                  Metrics* metrics) {
  chain->reserve(entry.certs.size());

  bool parsing_chain_succeeded = true;

  MetricsItem* parse_chain_metrics = metrics->GetAndIncrementTotal("Parse Certificate Chain");

  for (size_t i = 0; i < entry.certs.size(); ++i) {
    CertErrors errors;
    bool parsing_cert_succeeded =
        AddParsedCertificate(entry.certs[i]->AsInput(), chain, &errors);

    // Do extra logging for the leaf certificate (since intermediates are
    // iterated as leaves they will get logged too).
    if (i == 0) {
      MetricsItem* item = metrics->GetAndIncrementTotal("Parse Leaf Certificate");

      item->GetAndIncrementTotal(parsing_cert_succeeded ? "Success" : "Failure");

      LogCertErrors(errors, "Parse certificate Errors", entry.certs[i],
                    CertError::SEVERITY_HIGH, parsing_cert_succeeded, metrics);
      LogCertErrors(errors, "Parse certificate Warnings", entry.certs[i],
                    CertError::SEVERITY_WARNING, parsing_cert_succeeded, metrics);
    }

    if (!parsing_cert_succeeded) {
      parsing_chain_succeeded = false;
    }
  }

  if (parsing_chain_succeeded) {
    parse_chain_metrics->GetAndIncrementTotal("Success");
  } else {
    parse_chain_metrics->GetAndIncrementTotal("Failure")->AddSampleChain(entry);
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

class MyVisitor : public Visitor {
 public:
  MyVisitor() : delegate_(1024) {
    now_time_ = base::Time::Now();
    CHECK(der::EncodeTimeAsGeneralizedTime(now_time_, &now_));
    user_initial_policy_set_ = {AnyPolicy()};
  }

  void Visit(const Entry& entry, Metrics* metrics) override {
    // Count the entry types.
    {
      MetricsItem* item = metrics->GetAndIncrementTotal("Entry types");

      switch (entry.type) {
        case Entry::Type::kLeafCert:
          item->GetAndIncrementTotal("Leaf");
          break;
        case Entry::Type::kExtraCert:
          item->GetAndIncrementTotal("Extra Cert");
          break;
      }
    }

    // Parse all the certificate.
    ParsedCertificateList chain;
    if (!GetFullChain(entry, &chain, metrics))
      return;

    if (entry.type != Entry::Type::kLeafCert)
      return;

    base::Time not_before;
    base::Time not_after;

    GetChainExpiration(chain, &not_before, &not_after);
    bool chain_expired = now_time_ < not_before || now_time_ > not_after;

    if (chain_expired) {
      // Use the midpoint of the chain's validity.
      base::Time avg_t = not_before + (not_after - not_before) / 2;

      der::GeneralizedTime t;
      der::EncodeTimeAsGeneralizedTime(avg_t, &t);

      TestVerifyChain(chain, "[expired] ", entry, t, metrics);
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
    KeyPurpose required_key_purpose = KeyPurpose::SERVER_AUTH;
    CertPathErrors errors;
    VerifyCertificateChain(
        chain, CertificateTrust::ForTrustAnchor(), &delegate_,
        time, required_key_purpose, InitialExplicitPolicy::kFalse,
        user_initial_policy_set_, InitialPolicyMappingInhibit ::kFalse,
        InitialAnyPolicyInhibit::kFalse, nullptr, &errors);

    // Log whether validation succeeded.
    {
      MetricsItem* item =
          metrics->GetAndIncrementTotal(name_prefix + "Verify Chain");
      if (errors.ContainsHighSeverityErrors()) {
        item->GetAndIncrementTotal("Failure")->AddSampleChain(entry);
      } else {
        item->GetAndIncrementTotal("Success");
      }
    }

    // Log the breakdown of the errors.
    LogCertPathErrors(&errors, chain, name_prefix + "Verify Chain Errors",
                      entry, CertError::SEVERITY_HIGH, metrics);

    LogCertPathErrors(&errors, chain,
                      name_prefix + "Verify Chain Warnings", entry,
                      CertError::SEVERITY_WARNING, metrics);
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
