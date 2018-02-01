// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_TRIAL_COMPARISON_CERT_VERIFIER_H_
#define NET_CERT_TRIAL_COMPARISON_CERT_VERIFIER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "net/cert/cert_verifier.h"
#include "net/base/net_export.h"

namespace net {
class CertVerifyProc;
}

class NET_EXPORT TrialComparisonCertVerifier : public net::CertVerifier {
 public:
  TrialComparisonCertVerifier(
      void* profile_id,
      scoped_refptr<net::CertVerifyProc> primary_verify_proc,
      scoped_refptr<net::CertVerifyProc> trial_verify_proc);

  ~TrialComparisonCertVerifier() override;

  // CertVerifier implementation
  int Verify(const RequestParams& params,
             net::CRLSet* crl_set,
             net::CertVerifyResult* verify_result,
             const net::CompletionCallback& callback,
             std::unique_ptr<Request>* out_req,
             const net::NetLogWithSource& net_log) override;

  bool SupportsOCSPStapling() override;

 private:
  class TrialVerificationJob;

  void OnPrimaryVerifierComplete(const RequestParams& params,
                                 scoped_refptr<net::CRLSet> crl_set,
                                 const net::NetLogWithSource& net_log,
                                 int primary_error,
                                 const net::CertVerifyResult& primary_result,
                                 base::TimeDelta primary_latency);
  void MaybeDoTrialVerification(const RequestParams& params,
                                scoped_refptr<net::CRLSet> crl_set,
                                const net::NetLogWithSource& net_log,
                                int primary_error,
                                const net::CertVerifyResult& primary_result,
                                base::TimeDelta primary_latency,
                                void* profile_id,
                                bool trial_allowed);

  std::unique_ptr<TrialVerificationJob> RemoveJob(
      TrialVerificationJob* job_ptr);

  // The profile this verifier is associated with. Stored as a void* to avoid
  // accidentally using it on IO thread.
  void* profile_id_;

  std::unique_ptr<net::CertVerifier> primary_verifier_;
  std::unique_ptr<net::CertVerifier> trial_verifier_;

  std::map<TrialVerificationJob*, std::unique_ptr<TrialVerificationJob>> jobs_;

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<TrialComparisonCertVerifier> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrialComparisonCertVerifier);
};

#endif  // NET_CERT_TRIAL_COMPARISON_CERT_VERIFIER_H_
