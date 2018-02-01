// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/trial_comparison_cert_verifier.h"

#include <memory>

#include "base/bind.h"
#include "base/location.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_verify_proc.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/crl_set.h"
#include "net/cert/multi_threaded_cert_verifier.h"
#include "net/cert/x509_util.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"

//#include "chrome/browser/ssl/ssl_cert_reporter.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service_factory.h"
#include "components/certificate_reporting/error_report.h"
#include "components/certificate_reporting/cert_logger.pb.h"
#include "chrome/common/channel_info.h"

// XXX
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"

namespace {

bool CheckTrialEligibility(void* profile_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!g_browser_process->profile_manager()->IsValidProfile(profile_id)) {
    LOG(WARNING) << "CheckTrialEligibility profile_id not valid";
    return false;
  }
  const Profile* profile = reinterpret_cast<const Profile*>(profile_id);
  const PrefService& prefs = *profile->GetPrefs();
  // XXX check field trial status too
  // XXX check official build status?

  LOG(WARNING) << "profile->IsOffTheRecord()=" << profile->IsOffTheRecord()
	       << " IsScout=" << safe_browsing::IsScout(prefs)
	       << " IsExtendedReportingEnabled=" << safe_browsing::IsExtendedReportingEnabled(prefs);
  return !profile->IsOffTheRecord() && safe_browsing::IsScout(prefs) &&
         safe_browsing::IsExtendedReportingEnabled(prefs);
}

void SendReport(void* profile_id,
                const net::CertVerifier::RequestParams params,
                const net::CertVerifyResult primary_result,
                const net::CertVerifyResult trial_result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!g_browser_process->profile_manager()->IsValidProfile(profile_id)) {
    LOG(WARNING) << "SendReport profile_id not valid";
    return;
  }
  Profile* profile = reinterpret_cast<Profile*>(profile_id);
/*
  net::SSLInfo ssl_info;
  ssl_info.unverified_cert = params.certificate();

  ssl_info.cert = primary_result.verified_cert;
  ssl_info.cert_status = primary_result.cert_status;
  ssl_info.is_issued_by_known_root = primary_result.is_issued_by_known_root;
  ssl_info.public_key_hashes = primary_result.public_key_hashes;
  ssl_info.ocsp_result = primary_result.ocsp_result;
  certificate_reporting::ErrorReport report(params.hostname(), ssl_info);
*/

  certificate_reporting::ErrorReport report(
      params.hostname(), *params.certificate(), primary_result, trial_result);

  report.AddNetworkTimeInfo(g_browser_process->network_time_tracker());
  report.AddChromeChannel(chrome::GetChannel());

  std::string serialized_report;
  if (!report.Serialize(&serialized_report)) {
    LOG(ERROR) << "Serialize fail";
    return;
  }

  LOG(WARNING) << "Sending report...";
  CertificateReportingService* reporting_service =
      CertificateReportingServiceFactory::GetForBrowserContext(profile);
  DCHECK(reporting_service); // XXX?
  reporting_service->Send(serialized_report);
}

// XXX begin stuff copied from net/tools/cert_verify_tool/verify_using_cert_verify_proc.cc
// Associates a printable name with an integer constant. Useful for providing
// human-readable decoding of bitmask values.
struct StringToConstant {
  const char* name;
  const int constant;
};

const StringToConstant kCertStatusFlags[] = {
#define CERT_STATUS_FLAG(label, value) {#label, value},
#include "net/cert/cert_status_flags_list.h"
#undef CERT_STATUS_FLAG
};

// Returns a hex-encoded sha256 of the DER-encoding of |cert_handle|.
std::string FingerPrintCryptoBuffer(const CRYPTO_BUFFER* cert_handle) {
  net::SHA256HashValue hash =
      net::X509Certificate::CalculateFingerprint256(cert_handle);
  return base::HexEncode(hash.data, arraysize(hash.data));
}

// Returns a textual representation of the Subject of |cert|.
std::string SubjectFromX509Certificate(const net::X509Certificate* cert) {
  return cert->subject().GetDisplayName();
}

// Returns a textual representation of the Subject of |cert_handle|.
std::string SubjectFromCryptoBuffer(CRYPTO_BUFFER* cert_handle) {
  scoped_refptr<net::X509Certificate> cert =
      net::X509Certificate::CreateFromBuffer(
          net::x509_util::DupCryptoBuffer(cert_handle), {});
  if (!cert)
    return std::string();
  return SubjectFromX509Certificate(cert.get());
}

std::string PrintCertStatus(int cert_status) {
  std::string s = base::StringPrintf("CertStatus: 0x%x\n", cert_status);

  for (const auto& flag : kCertStatusFlags) {
    if ((cert_status & flag.constant) == flag.constant)
      s += std::string(" ") + flag.name + "\n";
  }
  return s;
}

std::string PrintCertVerifyResult(const net::CertVerifyResult& result) {
  std::string s = PrintCertStatus(result.cert_status);
  if (result.has_md2)
    s += "has_md2\n";
  if (result.has_md4)
    s += "has_md4\n";
  if (result.has_md5)
    s += "has_md5\n";
  if (result.has_sha1)
    s += "has_sha1\n";
  if (result.has_sha1_leaf)
    s += "has_sha1_leaf\n";
  if (result.is_issued_by_known_root)
    s += "is_issued_by_known_root\n";
  if (result.is_issued_by_additional_trust_anchor)
    s += "is_issued_by_additional_trust_anchor\n";

  if (result.verified_cert) {
    s += "chain:\n "
              + FingerPrintCryptoBuffer(result.verified_cert->cert_buffer())
              + " " + SubjectFromX509Certificate(result.verified_cert.get())
              + "\n";
    for (const auto& intermediate :
         result.verified_cert->intermediate_buffers()) {
      s += " " + FingerPrintCryptoBuffer(intermediate.get()) + " "
                + SubjectFromCryptoBuffer(intermediate.get()) + "\n";
    }
  }
  return s;
}
// XXX End of stuff copied from net/tools/cert_verify_tool/verify_using_cert_verify_proc.cc

}  // namespace

class TrialComparisonCertVerifier::TrialVerificationJob {
 public:
  TrialVerificationJob(const net::CertVerifier::RequestParams& params,
                       net::NetLog* net_log,
                       TrialComparisonCertVerifier* cert_verifier,
                       int primary_error,
                       const net::CertVerifyResult& primary_result,
                       base::TimeDelta primary_latency,
                       void* profile_id)
      : params_(params),
        start_time_(base::TimeTicks::Now()),
        net_log_(net::NetLogWithSource::Make(
            net_log,
            net::NetLogSourceType::TRIAL_CERT_VERIFIER_JOB)),  // XXX
        cert_verifier_(cert_verifier),
        primary_error_(primary_error),
        primary_result_(primary_result),
        primary_latency_(primary_latency),
        profile_id_(profile_id) {}

  int Start(net::CertVerifier* verifier, scoped_refptr<net::CRLSet> crl_set) {
    // XXX Unretained safe because ...
    // XXX Verify will make a new reference to crl_set if it needs one...
    int rv = verifier->Verify(params_, crl_set.get(), &trial_result_,
                             base::Bind(&TrialVerificationJob::OnJobCompleted,
                                        base::Unretained(this)),
                             &trial_request_, net_log_);
    if (rv != net::ERR_IO_PENDING)
      RecordTrialResults(rv);
    return rv;
  }

  void RecordTrialResults(int trial_result_error) {
    // XXX base::TimeDelta latency = base::TimeTicks::Now() - start_time_;
    bool ok = true;
    std::string hostname = params_.hostname();
    if (trial_result_error != primary_error_) {
      LOG(ERROR) << "for " << hostname << " primary error=" << primary_error_
                 << " trial verify error=" << trial_result_error;
      ok = false;
    }
    if (trial_result_ != primary_result_) {
      LOG(ERROR) << "CertVerifyResults differ for " << hostname;
      LOG(ERROR) << "primary CertVerifyResult: "
                 << PrintCertVerifyResult(primary_result_);
      LOG(ERROR) << "trial CertVerifyResult: "
                 << PrintCertVerifyResult(trial_result_);
      ok = false;
    }

    if (ok) {
      LOG(INFO) << "trial for comparison " << hostname << " matched!   yay";
      return;
    }

    content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)
        ->PostTask(FROM_HERE, base::Bind(&SendReport, profile_id_, params_,
                                         primary_result_, trial_result_));
  }

  void OnJobCompleted(int trial_result_error) {
    RecordTrialResults(trial_result_error);

    // |this| is deleted after the RemoveJob call.
    cert_verifier_->RemoveJob(this);
  }

 private:
  const net::CertVerifier::RequestParams params_;
  net::CertVerifyResult trial_result_;
  std::unique_ptr<net::CertVerifier::Request> trial_request_;

  // The tick count of when the job started. This is used to measure how long
  // the job actually took to complete.
  const base::TimeTicks start_time_;

  const net::NetLogWithSource net_log_;
  TrialComparisonCertVerifier* cert_verifier_;  // Non-owned.

  // Saved results of the primary verification.
  int primary_error_;
  const net::CertVerifyResult primary_result_;
  base::TimeDelta primary_latency_;

  void* profile_id_;

  // base::WeakPtrFactory<TrialVerificationJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrialVerificationJob);
};

TrialComparisonCertVerifier::TrialComparisonCertVerifier(
    void* profile_id,
    scoped_refptr<net::CertVerifyProc> primary_verify_proc,
    scoped_refptr<net::CertVerifyProc> trial_verify_proc)
    : profile_id_(profile_id),
      primary_verifier_(std::make_unique<net::MultiThreadedCertVerifier>(
          primary_verify_proc,
          // Unretained is safe since the callback won't be called after
          // |primary_verifier_| is destroyed.
          base::BindRepeating(
              &TrialComparisonCertVerifier::OnPrimaryVerifierComplete,
              base::Unretained(this)))),
      trial_verifier_(
          std::make_unique<net::MultiThreadedCertVerifier>(trial_verify_proc)),
      weak_ptr_factory_(this) {}

TrialComparisonCertVerifier::~TrialComparisonCertVerifier() = default;

int TrialComparisonCertVerifier::Verify(const RequestParams& params,
                                        net::CRLSet* crl_set,
                                        net::CertVerifyResult* verify_result,
                                        const net::CompletionCallback& callback,
                                        std::unique_ptr<Request>* out_req,
                                        const net::NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  return primary_verifier_->Verify(params, crl_set, verify_result, callback,
                                   out_req, net_log);
}

bool TrialComparisonCertVerifier::SupportsOCSPStapling() {
  return primary_verifier_->SupportsOCSPStapling();
}

void TrialComparisonCertVerifier::OnPrimaryVerifierComplete(
    const RequestParams& params,
    scoped_refptr<net::CRLSet> crl_set,
    const net::NetLogWithSource& net_log,
    int primary_error,
    const net::CertVerifyResult& primary_result,
    base::TimeDelta primary_latency) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  base::PostTaskAndReplyWithResult(
      content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)
          .get(),
      FROM_HERE, base::BindOnce(CheckTrialEligibility, profile_id_),
      base::BindOnce(&TrialComparisonCertVerifier::MaybeDoTrialVerification,
                     weak_ptr_factory_.GetWeakPtr(), params, std::move(crl_set),
                     net_log, primary_error, primary_result, primary_latency,
                     profile_id_));
}

void TrialComparisonCertVerifier::MaybeDoTrialVerification(
    const RequestParams& params,
    scoped_refptr<net::CRLSet> crl_set,
    const net::NetLogWithSource& net_log,
    int primary_error,
    const net::CertVerifyResult& primary_result,
    base::TimeDelta primary_latency,
    void* profile_id,
    bool trial_allowed) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!trial_allowed) {
    LOG(WARNING) << "trial not allowed";
    return;
  }

  std::unique_ptr<TrialVerificationJob> job =
      std::make_unique<TrialVerificationJob>(params, net_log.net_log(), this,
                                             primary_error, primary_result,
                                             primary_latency, profile_id);

  if (job->Start(trial_verifier_.get(), std::move(crl_set)) == net::ERR_IO_PENDING) {
    TrialVerificationJob* job_ptr = job.get();
    jobs_[job_ptr] = std::move(job);
  }
}

std::unique_ptr<TrialComparisonCertVerifier::TrialVerificationJob>
TrialComparisonCertVerifier::RemoveJob(TrialVerificationJob* job_ptr) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto it = jobs_.find(job_ptr);
  DCHECK(it != jobs_.end());
  std::unique_ptr<TrialVerificationJob> job = std::move(it->second);
  jobs_.erase(it);
  return job;
}
