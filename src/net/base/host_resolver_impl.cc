// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/host_resolver_impl.h"

#if defined(OS_WIN)
#include <Winsock2.h>
#elif defined(OS_POSIX)
#include <netdb.h>
#endif

#include <cmath>
#include <utility>
#include <vector>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/debug/debugger.h"
#include "base/debug/stack_trace.h"
#include "base/message_loop_proxy.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/stl_util.h"
#include "base/string_util.h"
#include "base/threading/worker_pool.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "net/base/address_list.h"
#include "net/base/address_list_net_log_param.h"
#include "net/base/dns_reloader.h"
#include "net/base/host_port_pair.h"
#include "net/base/host_resolver_proc.h"
#include "net/base/net_errors.h"
#include "net/base/net_log.h"
#include "net/base/net_util.h"

#if defined(OS_WIN)
#include "net/base/winsock_init.h"
#endif

namespace net {

namespace {

// Limit the size of hostnames that will be resolved to combat issues in
// some platform's resolvers.
const size_t kMaxHostLength = 4096;

// Default TTL for successful resolutions with ProcTask.
const unsigned kCacheEntryTTLSeconds = 60;

// Maximum of 8 concurrent resolver threads (excluding retries).
// Some routers (or resolvers) appear to start to provide host-not-found if
// too many simultaneous resolutions are pending.  This number needs to be
// further optimized, but 8 is what FF currently does.
static const size_t kDefaultMaxProcTasks = 8u;

// Helper to mutate the linked list contained by AddressList to the given
// port. Note that in general this is dangerous since the AddressList's
// data might be shared (and you should use AddressList::SetPort).
//
// However since we allocated the AddressList ourselves we can safely
// do this optimization and avoid reallocating the list.
void MutableSetPort(int port, AddressList* addrlist) {
  struct addrinfo* mutable_head =
      const_cast<struct addrinfo*>(addrlist->head());
  SetPortForAllAddrinfos(mutable_head, port);
}

// We use a separate histogram name for each platform to facilitate the
// display of error codes by their symbolic name (since each platform has
// different mappings).
const char kOSErrorsForGetAddrinfoHistogramName[] =
#if defined(OS_WIN)
    "Net.OSErrorsForGetAddrinfo_Win";
#elif defined(OS_MACOSX)
    "Net.OSErrorsForGetAddrinfo_Mac";
#elif defined(OS_LINUX)
    "Net.OSErrorsForGetAddrinfo_Linux";
#else
    "Net.OSErrorsForGetAddrinfo";
#endif

// Gets a list of the likely error codes that getaddrinfo() can return
// (non-exhaustive). These are the error codes that we will track via
// a histogram.
std::vector<int> GetAllGetAddrinfoOSErrors() {
  int os_errors[] = {
#if defined(OS_POSIX)
#if !defined(OS_FREEBSD)
#if !defined(OS_ANDROID)
    // EAI_ADDRFAMILY has been declared obsolete in Android's and
    // FreeBSD's netdb.h.
    EAI_ADDRFAMILY,
#endif
    // EAI_NODATA has been declared obsolete in FreeBSD's netdb.h.
    EAI_NODATA,
#endif
    EAI_AGAIN,
    EAI_BADFLAGS,
    EAI_FAIL,
    EAI_FAMILY,
    EAI_MEMORY,
    EAI_NONAME,
    EAI_SERVICE,
    EAI_SOCKTYPE,
    EAI_SYSTEM,
#elif defined(OS_WIN)
    // See: http://msdn.microsoft.com/en-us/library/ms738520(VS.85).aspx
    WSA_NOT_ENOUGH_MEMORY,
    WSAEAFNOSUPPORT,
    WSAEINVAL,
    WSAESOCKTNOSUPPORT,
    WSAHOST_NOT_FOUND,
    WSANO_DATA,
    WSANO_RECOVERY,
    WSANOTINITIALISED,
    WSATRY_AGAIN,
    WSATYPE_NOT_FOUND,
    // The following are not in doc, but might be to appearing in results :-(.
    WSA_INVALID_HANDLE,
#endif
  };

  // Ensure all errors are positive, as histogram only tracks positive values.
  for (size_t i = 0; i < arraysize(os_errors); ++i) {
    os_errors[i] = std::abs(os_errors[i]);
  }

  return base::CustomHistogram::ArrayToCustomRanges(os_errors,
                                                    arraysize(os_errors));
}

// Wraps call to SystemHostResolverProc as an instance of HostResolverProc.
// TODO(szym): This should probably be declared in host_resolver_proc.h.
class CallSystemHostResolverProc : public HostResolverProc {
 public:
  CallSystemHostResolverProc() : HostResolverProc(NULL) {}
  virtual int Resolve(const std::string& hostname,
                      AddressFamily address_family,
                      HostResolverFlags host_resolver_flags,
                      AddressList* addrlist,
                      int* os_error) OVERRIDE {
    return SystemHostResolverProc(hostname,
                                  address_family,
                                  host_resolver_flags,
                                  addrlist,
                                  os_error);
  }
};

// Extra parameters to attach to the NetLog when the resolve failed.
class HostResolveFailedParams : public NetLog::EventParameters {
 public:
  HostResolveFailedParams(uint32 attempt_number,
                          int net_error,
                          int os_error)
      : attempt_number_(attempt_number),
        net_error_(net_error),
        os_error_(os_error) {
  }

  virtual Value* ToValue() const OVERRIDE {
    DictionaryValue* dict = new DictionaryValue();
    if (attempt_number_)
      dict->SetInteger("attempt_number", attempt_number_);

    dict->SetInteger("net_error", net_error_);

    if (os_error_) {
      dict->SetInteger("os_error", os_error_);
#if defined(OS_POSIX)
      dict->SetString("os_error_string", gai_strerror(os_error_));
#elif defined(OS_WIN)
      // Map the error code to a human-readable string.
      LPWSTR error_string = NULL;
      int size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM,
                               0,  // Use the internal message table.
                               os_error_,
                               0,  // Use default language.
                               (LPWSTR)&error_string,
                               0,  // Buffer size.
                               0);  // Arguments (unused).
      dict->SetString("os_error_string", WideToUTF8(error_string));
      LocalFree(error_string);
#endif
    }

    return dict;
  }

 private:
  const uint32 attempt_number_;
  const int net_error_;
  const int os_error_;
};

// Parameters representing the information in a RequestInfo object, along with
// the associated NetLog::Source.
class RequestInfoParameters : public NetLog::EventParameters {
 public:
  RequestInfoParameters(const HostResolver::RequestInfo& info,
                        const NetLog::Source& source)
      : info_(info), source_(source) {}

  virtual Value* ToValue() const OVERRIDE {
    DictionaryValue* dict = new DictionaryValue();
    dict->SetString("host", info_.host_port_pair().ToString());
    dict->SetInteger("address_family",
                     static_cast<int>(info_.address_family()));
    dict->SetBoolean("allow_cached_response", info_.allow_cached_response());
    dict->SetBoolean("is_speculative", info_.is_speculative());
    dict->SetInteger("priority", info_.priority());

    if (source_.is_valid())
      dict->Set("source_dependency", source_.ToValue());

    return dict;
  }

 private:
  const HostResolver::RequestInfo info_;
  const NetLog::Source source_;
};

// Parameters associated with the creation of a HostResolverImpl::Job
// or a HostResolverImpl::ProcTask.
class JobCreationParameters : public NetLog::EventParameters {
 public:
  JobCreationParameters(const std::string& host,
                        const NetLog::Source& source)
      : host_(host), source_(source) {}

  virtual Value* ToValue() const OVERRIDE {
    DictionaryValue* dict = new DictionaryValue();
    dict->SetString("host", host_);
    dict->Set("source_dependency", source_.ToValue());
    return dict;
  }

 private:
  const std::string host_;
  const NetLog::Source source_;
};

// Parameters of the HOST_RESOLVER_IMPL_JOB_ATTACH/DETACH event.
class JobAttachParameters : public NetLog::EventParameters {
 public:
  JobAttachParameters(const NetLog::Source& source,
                      RequestPriority priority)
      : source_(source), priority_(priority) {}

  virtual Value* ToValue() const OVERRIDE {
    DictionaryValue* dict = new DictionaryValue();
    dict->Set("source_dependency", source_.ToValue());
    dict->SetInteger("priority", priority_);
    return dict;
  }

 private:
  const NetLog::Source source_;
  const RequestPriority priority_;
};

// The logging routines are defined here because some requests are resolved
// without a Request object.

// Logs when a request has just been started.
void LogStartRequest(const BoundNetLog& source_net_log,
                     const BoundNetLog& request_net_log,
                     const HostResolver::RequestInfo& info) {
  source_net_log.BeginEvent(
      NetLog::TYPE_HOST_RESOLVER_IMPL,
      make_scoped_refptr(new NetLogSourceParameter(
          "source_dependency", request_net_log.source())));

  request_net_log.BeginEvent(
      NetLog::TYPE_HOST_RESOLVER_IMPL_REQUEST,
      make_scoped_refptr(new RequestInfoParameters(
          info, source_net_log.source())));
}

// Logs when a request has just completed (before its callback is run).
void LogFinishRequest(const BoundNetLog& source_net_log,
                      const BoundNetLog& request_net_log,
                      const HostResolver::RequestInfo& info,
                      int net_error,
                      int os_error) {
  scoped_refptr<NetLog::EventParameters> params;
  if (net_error != OK) {
    params = new HostResolveFailedParams(0, net_error, os_error);
  }

  request_net_log.EndEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_REQUEST, params);
  source_net_log.EndEvent(NetLog::TYPE_HOST_RESOLVER_IMPL, NULL);
}

// Logs when a request has been cancelled.
void LogCancelRequest(const BoundNetLog& source_net_log,
                      const BoundNetLog& request_net_log,
                      const HostResolverImpl::RequestInfo& info) {
  request_net_log.AddEvent(NetLog::TYPE_CANCELLED, NULL);
  request_net_log.EndEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_REQUEST, NULL);
  source_net_log.EndEvent(NetLog::TYPE_HOST_RESOLVER_IMPL, NULL);
}

//-----------------------------------------------------------------------------

// Keeps track of the highest priority.
class PriorityTracker {
 public:
  PriorityTracker()
      : highest_priority_(IDLE), total_count_(0) {
    memset(counts_, 0, sizeof(counts_));
  }

  RequestPriority highest_priority() const {
    return highest_priority_;
  }

  size_t total_count() const {
    return total_count_;
  }

  void Add(RequestPriority req_priority) {
    ++total_count_;
    ++counts_[req_priority];
    if (highest_priority_ > req_priority)
      highest_priority_ = req_priority;
  }

  void Remove(RequestPriority req_priority) {
    DCHECK_GT(total_count_, 0u);
    DCHECK_GT(counts_[req_priority], 0u);
    --total_count_;
    --counts_[req_priority];
    size_t i;
    for (i = highest_priority_; i < NUM_PRIORITIES && !counts_[i]; ++i);
    highest_priority_ = static_cast<RequestPriority>(i);

    // In absence of requests set default.
    if (highest_priority_ == NUM_PRIORITIES) {
      DCHECK_EQ(0u, total_count_);
      highest_priority_ = IDLE;
    }
  }

 private:
  RequestPriority highest_priority_;
  size_t total_count_;
  size_t counts_[NUM_PRIORITIES];
};

//-----------------------------------------------------------------------------

HostResolver* CreateHostResolver(size_t max_concurrent_resolves,
                                 size_t max_retry_attempts,
                                 bool use_cache,
                                 NetLog* net_log) {
  if (max_concurrent_resolves == HostResolver::kDefaultParallelism)
    max_concurrent_resolves = kDefaultMaxProcTasks;

  // TODO(szym): Add experiments with reserved slots for higher priority
  // requests.

  PrioritizedDispatcher::Limits limits(NUM_PRIORITIES, max_concurrent_resolves);

  HostResolverImpl* resolver = new HostResolverImpl(
      use_cache ? HostCache::CreateDefaultCache() : NULL,
      limits,
      HostResolverImpl::ProcTaskParams(NULL, max_retry_attempts),
      net_log);

  return resolver;
}

}  // anonymous namespace

//-----------------------------------------------------------------------------

HostResolver* CreateSystemHostResolver(size_t max_concurrent_resolves,
                                       size_t max_retry_attempts,
                                       NetLog* net_log) {
  return CreateHostResolver(max_concurrent_resolves,
                            max_retry_attempts,
                            true /* use_cache */,
                            net_log);
}

HostResolver* CreateNonCachingSystemHostResolver(size_t max_concurrent_resolves,
                                                 size_t max_retry_attempts,
                                                 NetLog* net_log) {
  return CreateHostResolver(max_concurrent_resolves,
                            max_retry_attempts,
                            false /* use_cache */,
                            net_log);
}

//-----------------------------------------------------------------------------

// Holds the data for a request that could not be completed synchronously.
// It is owned by a Job. Canceled Requests are only marked as canceled rather
// than removed from the Job's |requests_| list.
class HostResolverImpl::Request {
 public:
  Request(const BoundNetLog& source_net_log,
          const BoundNetLog& request_net_log,
          const RequestInfo& info,
          const CompletionCallback& callback,
          AddressList* addresses)
      : source_net_log_(source_net_log),
        request_net_log_(request_net_log),
        info_(info),
        job_(NULL),
        callback_(callback),
        addresses_(addresses) {
  }

  // Mark the request as canceled.
  void MarkAsCanceled() {
    job_ = NULL;
    addresses_ = NULL;
    callback_.Reset();
  }

  bool was_canceled() const {
    return callback_.is_null();
  }

  void set_job(Job* job) {
    DCHECK(job);
    // Identify which job the request is waiting on.
    job_ = job;
  }

  // Prepare final AddressList and call completion callback.
  void OnComplete(int error, const AddressList& addrlist) {
    if (error == OK)
      *addresses_ = CreateAddressListUsingPort(addrlist, info_.port());
    CompletionCallback callback = callback_;
    MarkAsCanceled();
    callback.Run(error);
  }

  Job* job() const {
    return job_;
  }

  // NetLog for the source, passed in HostResolver::Resolve.
  const BoundNetLog& source_net_log() {
    return source_net_log_;
  }

  // NetLog for this request.
  const BoundNetLog& request_net_log() {
    return request_net_log_;
  }

  const RequestInfo& info() const {
    return info_;
  }

 private:
  BoundNetLog source_net_log_;
  BoundNetLog request_net_log_;

  // The request info that started the request.
  RequestInfo info_;

  // The resolve job that this request is dependent on.
  Job* job_;

  // The user's callback to invoke when the request completes.
  CompletionCallback callback_;

  // The address list to save result into.
  AddressList* addresses_;

  DISALLOW_COPY_AND_ASSIGN(Request);
};

//------------------------------------------------------------------------------

// Provide a common macro to simplify code and readability. We must use a
// macros as the underlying HISTOGRAM macro creates static varibles.
#define DNS_HISTOGRAM(name, time) UMA_HISTOGRAM_CUSTOM_TIMES(name, time, \
    base::TimeDelta::FromMicroseconds(1), base::TimeDelta::FromHours(1), 100)

// Calls HostResolverProc on the WorkerPool. Performs retries if necessary.
//
// Whenever we try to resolve the host, we post a delayed task to check if host
// resolution (OnLookupComplete) is completed or not. If the original attempt
// hasn't completed, then we start another attempt for host resolution. We take
// the results from the first attempt that finishes and ignore the results from
// all other attempts.
//
// TODO(szym): Move to separate source file for testing and mocking.
//
class HostResolverImpl::ProcTask
    : public base::RefCountedThreadSafe<HostResolverImpl::ProcTask> {
 public:
  typedef base::Callback<void(int, int, const AddressList&)> Callback;

  ProcTask(const Key& key,
           const ProcTaskParams& params,
           const Callback& callback,
           const BoundNetLog& job_net_log)
      : key_(key),
        params_(params),
        callback_(callback),
        origin_loop_(base::MessageLoopProxy::current()),
        attempt_number_(0),
        completed_attempt_number_(0),
        completed_attempt_error_(ERR_UNEXPECTED),
        had_non_speculative_request_(false),
        net_log_(BoundNetLog::Make(
            job_net_log.net_log(),
            NetLog::SOURCE_HOST_RESOLVER_IMPL_PROC_TASK)) {
    if (!params_.resolver_proc)
      params_.resolver_proc = HostResolverProc::GetDefault();
    // If default is unset, use the system proc.
    if (!params_.resolver_proc)
      params_.resolver_proc = new CallSystemHostResolverProc();

    job_net_log.AddEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_CREATE_PROC_TASK,
                         new NetLogSourceParameter("source_dependency",
                                                   net_log_.source()));

    net_log_.BeginEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_PROC_TASK,
                        new JobCreationParameters(key_.hostname,
                                                  job_net_log.source()));
  }

  void Start() {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    StartLookupAttempt();
  }

  // Cancels this ProcTask. It will be orphaned. Any outstanding resolve
  // attempts running on worker threads will continue running. Only once all the
  // attempts complete will the final reference to this ProcTask be released.
  void Cancel() {
    DCHECK(origin_loop_->BelongsToCurrentThread());

    if (was_canceled())
      return;

    net_log_.AddEvent(NetLog::TYPE_CANCELLED, NULL);

    callback_.Reset();

    net_log_.EndEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_PROC_TASK, NULL);
  }

  void set_had_non_speculative_request() {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    had_non_speculative_request_ = true;
  }

  bool was_canceled() const {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    return callback_.is_null();
  }

  bool was_completed() const {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    return completed_attempt_number_ > 0;
  }

 private:
  void StartLookupAttempt() {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    base::TimeTicks start_time = base::TimeTicks::Now();
    ++attempt_number_;
    // Dispatch the lookup attempt to a worker thread.
    if (!base::WorkerPool::PostTask(
            FROM_HERE,
            base::Bind(&ProcTask::DoLookup, this, start_time, attempt_number_),
            true)) {
      NOTREACHED();

      // Since we could be running within Resolve() right now, we can't just
      // call OnLookupComplete().  Instead we must wait until Resolve() has
      // returned (IO_PENDING).
      origin_loop_->PostTask(
          FROM_HERE,
          base::Bind(&ProcTask::OnLookupComplete, this, AddressList(),
                     start_time, attempt_number_, ERR_UNEXPECTED, 0));
      return;
    }

    net_log_.AddEvent(
        NetLog::TYPE_HOST_RESOLVER_IMPL_ATTEMPT_STARTED,
        make_scoped_refptr(new NetLogIntegerParameter(
            "attempt_number", attempt_number_)));

    // If we don't get the results within a given time, RetryIfNotComplete
    // will start a new attempt on a different worker thread if none of our
    // outstanding attempts have completed yet.
    if (attempt_number_ <= params_.max_retry_attempts) {
      origin_loop_->PostDelayedTask(
          FROM_HERE,
          base::Bind(&ProcTask::RetryIfNotComplete, this),
          params_.unresponsive_delay.InMilliseconds());
    }
  }

  // WARNING: This code runs inside a worker pool. The shutdown code cannot
  // wait for it to finish, so we must be very careful here about using other
  // objects (like MessageLoops, Singletons, etc). During shutdown these objects
  // may no longer exist. Multiple DoLookups() could be running in parallel, so
  // any state inside of |this| must not mutate .
  void DoLookup(const base::TimeTicks& start_time,
                const uint32 attempt_number) {
    AddressList results;
    int os_error = 0;
    // Running on the worker thread

    int error = params_.resolver_proc->Resolve(key_.hostname,
                                               key_.address_family,
                                               key_.host_resolver_flags,
                                               &results,
                                               &os_error);

    origin_loop_->PostTask(
        FROM_HERE,
        base::Bind(&ProcTask::OnLookupComplete, this, results, start_time,
                   attempt_number, error, os_error));
  }

  // Makes next attempt if DoLookup() has not finished (runs on origin thread).
  void RetryIfNotComplete() {
    DCHECK(origin_loop_->BelongsToCurrentThread());

    if (was_completed() || was_canceled())
      return;

    params_.unresponsive_delay *= params_.retry_factor;
    StartLookupAttempt();
  }

  // Callback for when DoLookup() completes (runs on origin thread).
  void OnLookupComplete(const AddressList& results,
                        const base::TimeTicks& start_time,
                        const uint32 attempt_number,
                        int error,
                        const int os_error) {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    DCHECK(error || results.head());

    bool was_retry_attempt = attempt_number > 1;

    // Ideally the following code would be part of host_resolver_proc.cc,
    // however it isn't safe to call NetworkChangeNotifier from worker
    // threads. So we do it here on the IO thread instead.
    if (error != OK && NetworkChangeNotifier::IsOffline())
      error = ERR_INTERNET_DISCONNECTED;

    // If this is the first attempt that is finishing later, then record
    // data for the first attempt. Won't contaminate with retry attempt's
    // data.
    if (!was_retry_attempt)
      RecordPerformanceHistograms(start_time, error, os_error);

    RecordAttemptHistograms(start_time, attempt_number, error, os_error);

    if (was_canceled())
      return;

    scoped_refptr<NetLog::EventParameters> params;
    if (error != OK) {
      params = new HostResolveFailedParams(attempt_number, error, os_error);
    } else {
      params = new NetLogIntegerParameter("attempt_number", attempt_number);
    }
    net_log_.AddEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_ATTEMPT_FINISHED, params);

    if (was_completed())
      return;

    // Copy the results from the first worker thread that resolves the host.
    results_ = results;
    completed_attempt_number_ = attempt_number;
    completed_attempt_error_ = error;

    if (was_retry_attempt) {
      // If retry attempt finishes before 1st attempt, then get stats on how
      // much time is saved by having spawned an extra attempt.
      retry_attempt_finished_time_ = base::TimeTicks::Now();
    }

    if (error != OK) {
      params = new HostResolveFailedParams(0, error, os_error);
    } else {
      params = new AddressListNetLogParam(results_);
    }
    net_log_.EndEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_PROC_TASK, params);

    callback_.Run(error, os_error, results_);
  }

  void RecordPerformanceHistograms(const base::TimeTicks& start_time,
                                   const int error,
                                   const int os_error) const {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    enum Category {  // Used in HISTOGRAM_ENUMERATION.
      RESOLVE_SUCCESS,
      RESOLVE_FAIL,
      RESOLVE_SPECULATIVE_SUCCESS,
      RESOLVE_SPECULATIVE_FAIL,
      RESOLVE_MAX,  // Bounding value.
    };
    int category = RESOLVE_MAX;  // Illegal value for later DCHECK only.

    base::TimeDelta duration = base::TimeTicks::Now() - start_time;
    if (error == OK) {
      if (had_non_speculative_request_) {
        category = RESOLVE_SUCCESS;
        DNS_HISTOGRAM("DNS.ResolveSuccess", duration);
      } else {
        category = RESOLVE_SPECULATIVE_SUCCESS;
        DNS_HISTOGRAM("DNS.ResolveSpeculativeSuccess", duration);
      }

      // Log DNS lookups based on address_family.  This will help us determine
      // if IPv4 or IPv4/6 lookups are faster or slower.
      switch(key_.address_family) {
        case ADDRESS_FAMILY_IPV4:
          DNS_HISTOGRAM("DNS.ResolveSuccess_FAMILY_IPV4", duration);
          break;
        case ADDRESS_FAMILY_IPV6:
          DNS_HISTOGRAM("DNS.ResolveSuccess_FAMILY_IPV6", duration);
          break;
        case ADDRESS_FAMILY_UNSPECIFIED:
          DNS_HISTOGRAM("DNS.ResolveSuccess_FAMILY_UNSPEC", duration);
          break;
      }
    } else {
      if (had_non_speculative_request_) {
        category = RESOLVE_FAIL;
        DNS_HISTOGRAM("DNS.ResolveFail", duration);
      } else {
        category = RESOLVE_SPECULATIVE_FAIL;
        DNS_HISTOGRAM("DNS.ResolveSpeculativeFail", duration);
      }
      // Log DNS lookups based on address_family.  This will help us determine
      // if IPv4 or IPv4/6 lookups are faster or slower.
      switch(key_.address_family) {
        case ADDRESS_FAMILY_IPV4:
          DNS_HISTOGRAM("DNS.ResolveFail_FAMILY_IPV4", duration);
          break;
        case ADDRESS_FAMILY_IPV6:
          DNS_HISTOGRAM("DNS.ResolveFail_FAMILY_IPV6", duration);
          break;
        case ADDRESS_FAMILY_UNSPECIFIED:
          DNS_HISTOGRAM("DNS.ResolveFail_FAMILY_UNSPEC", duration);
          break;
      }
      UMA_HISTOGRAM_CUSTOM_ENUMERATION(kOSErrorsForGetAddrinfoHistogramName,
                                       std::abs(os_error),
                                       GetAllGetAddrinfoOSErrors());
    }
    DCHECK_LT(category, static_cast<int>(RESOLVE_MAX));  // Be sure it was set.

    UMA_HISTOGRAM_ENUMERATION("DNS.ResolveCategory", category, RESOLVE_MAX);

    static const bool show_speculative_experiment_histograms =
        base::FieldTrialList::TrialExists("DnsImpact");
    if (show_speculative_experiment_histograms) {
      UMA_HISTOGRAM_ENUMERATION(
          base::FieldTrial::MakeName("DNS.ResolveCategory", "DnsImpact"),
          category, RESOLVE_MAX);
      if (RESOLVE_SUCCESS == category) {
        DNS_HISTOGRAM(base::FieldTrial::MakeName("DNS.ResolveSuccess",
                                                 "DnsImpact"), duration);
      }
    }
    static const bool show_parallelism_experiment_histograms =
        base::FieldTrialList::TrialExists("DnsParallelism");
    if (show_parallelism_experiment_histograms) {
      UMA_HISTOGRAM_ENUMERATION(
          base::FieldTrial::MakeName("DNS.ResolveCategory", "DnsParallelism"),
          category, RESOLVE_MAX);
      if (RESOLVE_SUCCESS == category) {
        DNS_HISTOGRAM(base::FieldTrial::MakeName("DNS.ResolveSuccess",
                                                 "DnsParallelism"), duration);
      }
    }
  }

  void RecordAttemptHistograms(const base::TimeTicks& start_time,
                               const uint32 attempt_number,
                               const int error,
                               const int os_error) const {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    bool first_attempt_to_complete =
        completed_attempt_number_ == attempt_number;
    bool is_first_attempt = (attempt_number == 1);

    if (first_attempt_to_complete) {
      // If this was first attempt to complete, then record the resolution
      // status of the attempt.
      if (completed_attempt_error_ == OK) {
        UMA_HISTOGRAM_ENUMERATION(
            "DNS.AttemptFirstSuccess", attempt_number, 100);
      } else {
        UMA_HISTOGRAM_ENUMERATION(
            "DNS.AttemptFirstFailure", attempt_number, 100);
      }
    }

    if (error == OK)
      UMA_HISTOGRAM_ENUMERATION("DNS.AttemptSuccess", attempt_number, 100);
    else
      UMA_HISTOGRAM_ENUMERATION("DNS.AttemptFailure", attempt_number, 100);

    // If first attempt didn't finish before retry attempt, then calculate stats
    // on how much time is saved by having spawned an extra attempt.
    if (!first_attempt_to_complete && is_first_attempt && !was_canceled()) {
      DNS_HISTOGRAM("DNS.AttemptTimeSavedByRetry",
                    base::TimeTicks::Now() - retry_attempt_finished_time_);
    }

    if (was_canceled() || !first_attempt_to_complete) {
      // Count those attempts which completed after the job was already canceled
      // OR after the job was already completed by an earlier attempt (so in
      // effect).
      UMA_HISTOGRAM_ENUMERATION("DNS.AttemptDiscarded", attempt_number, 100);

      // Record if job is canceled.
      if (was_canceled())
        UMA_HISTOGRAM_ENUMERATION("DNS.AttemptCancelled", attempt_number, 100);
    }

    base::TimeDelta duration = base::TimeTicks::Now() - start_time;
    if (error == OK)
      DNS_HISTOGRAM("DNS.AttemptSuccessDuration", duration);
    else
      DNS_HISTOGRAM("DNS.AttemptFailDuration", duration);
  }

  // Set on the origin thread, read on the worker thread.
  Key key_;

  // Holds an owning reference to the HostResolverProc that we are going to use.
  // This may not be the current resolver procedure by the time we call
  // ResolveAddrInfo, but that's OK... we'll use it anyways, and the owning
  // reference ensures that it remains valid until we are done.
  ProcTaskParams params_;

  // The listener to the results of this ProcTask.
  Callback callback_;

  // Used to post ourselves onto the origin thread.
  scoped_refptr<base::MessageLoopProxy> origin_loop_;

  // Keeps track of the number of attempts we have made so far to resolve the
  // host. Whenever we start an attempt to resolve the host, we increase this
  // number.
  uint32 attempt_number_;

  // The index of the attempt which finished first (or 0 if the job is still in
  // progress).
  uint32 completed_attempt_number_;

  // The result (a net error code) from the first attempt to complete.
  int completed_attempt_error_;

  // The time when retry attempt was finished.
  base::TimeTicks retry_attempt_finished_time_;

  // True if a non-speculative request was ever attached to this job
  // (regardless of whether or not it was later canceled.
  // This boolean is used for histogramming the duration of jobs used to
  // service non-speculative requests.
  bool had_non_speculative_request_;

  AddressList results_;

  BoundNetLog net_log_;

  DISALLOW_COPY_AND_ASSIGN(ProcTask);
};

//-----------------------------------------------------------------------------

// Represents a request to the worker pool for a "probe for IPv6 support" call.
class HostResolverImpl::IPv6ProbeJob
    : public base::RefCountedThreadSafe<HostResolverImpl::IPv6ProbeJob> {
 public:
  explicit IPv6ProbeJob(HostResolverImpl* resolver)
      : resolver_(resolver),
        origin_loop_(base::MessageLoopProxy::current()) {
    DCHECK(resolver);
  }

  void Start() {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    if (was_canceled())
      return;
    const bool kIsSlow = true;
    base::WorkerPool::PostTask(
        FROM_HERE, base::Bind(&IPv6ProbeJob::DoProbe, this), kIsSlow);
  }

  // Cancels the current job.
  void Cancel() {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    if (was_canceled())
      return;
    resolver_ = NULL;  // Read/write ONLY on origin thread.
  }

 private:
  friend class base::RefCountedThreadSafe<HostResolverImpl::IPv6ProbeJob>;

  ~IPv6ProbeJob() {
  }

  bool was_canceled() const {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    return !resolver_;
  }

  // Run on worker thread.
  void DoProbe() {
    // Do actual testing on this thread, as it takes 40-100ms.
    AddressFamily family = IPv6Supported() ? ADDRESS_FAMILY_UNSPECIFIED
                                           : ADDRESS_FAMILY_IPV4;

    origin_loop_->PostTask(
        FROM_HERE,
        base::Bind(&IPv6ProbeJob::OnProbeComplete, this, family));
  }

  // Callback for when DoProbe() completes.
  void OnProbeComplete(AddressFamily address_family) {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    if (was_canceled())
      return;
    resolver_->IPv6ProbeSetDefaultAddressFamily(address_family);
  }

  // Used/set only on origin thread.
  HostResolverImpl* resolver_;

  // Used to post ourselves onto the origin thread.
  scoped_refptr<base::MessageLoopProxy> origin_loop_;

  DISALLOW_COPY_AND_ASSIGN(IPv6ProbeJob);
};

//-----------------------------------------------------------------------------

// Aggregates all Requests for the same Key. Dispatched via PriorityDispatch.
// Spawns ProcTask when started.
class HostResolverImpl::Job : public PrioritizedDispatcher::Job {
 public:
  // Creates new job for |key| where |request_net_log| is bound to the
  // request that spawned it.
  Job(HostResolverImpl* resolver,
      const Key& key,
      const BoundNetLog& request_net_log)
      : resolver_(resolver->AsWeakPtr()),
        key_(key),
        had_non_speculative_request_(false),
        net_log_(BoundNetLog::Make(request_net_log.net_log(),
                                   NetLog::SOURCE_HOST_RESOLVER_IMPL_JOB)),
        net_error_(ERR_IO_PENDING),
        os_error_(0) {
    request_net_log.AddEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_CREATE_JOB, NULL);

    net_log_.BeginEvent(
        NetLog::TYPE_HOST_RESOLVER_IMPL_JOB,
        make_scoped_refptr(new JobCreationParameters(
            key_.hostname, request_net_log.source())));
  }

  virtual ~Job() {
    if (net_error_ == ERR_IO_PENDING) {
      if (is_running()) {
        DCHECK_EQ(ERR_IO_PENDING, net_error_);
        proc_task_->Cancel();
        proc_task_ = NULL;
        net_error_ = ERR_ABORTED;
      } else {
        net_log_.AddEvent(NetLog::TYPE_CANCELLED, NULL);
        net_error_ = OK;  // For NetLog.
      }

      for (RequestsList::const_iterator it = requests_.begin();
           it != requests_.end(); ++it) {
        Request* req = *it;
        if (req->was_canceled())
          continue;
        DCHECK_EQ(this, req->job());
        LogCancelRequest(req->source_net_log(), req->request_net_log(),
                         req->info());
      }
    }
    net_log_.EndEventWithNetErrorCode(NetLog::TYPE_HOST_RESOLVER_IMPL_JOB,
                                      net_error_);
    STLDeleteElements(&requests_);
  }

  HostResolverImpl* resolver() const {
    return resolver_;
  }

  RequestPriority priority() const {
    return priority_tracker_.highest_priority();
  }

  // Number of non-canceled requests in |requests_|.
  size_t num_active_requests() const {
    return priority_tracker_.total_count();
  }

  const Key& key() const {
    return key_;
  }

  int net_error() const {
    return net_error_;
  }

  // Used by HostResolverImpl with |dispatcher_|.
  const PrioritizedDispatcher::Handle& handle() const {
    return handle_;
  }

  void set_handle(const PrioritizedDispatcher::Handle& handle) {
    handle_ = handle;
  }

  // The Job will own |req| and destroy it in ~Job.
  void AddRequest(Request* req) {
    DCHECK_EQ(key_.hostname, req->info().hostname());

    req->set_job(this);
    requests_.push_back(req);

    priority_tracker_.Add(req->info().priority());

    req->request_net_log().AddEvent(
        NetLog::TYPE_HOST_RESOLVER_IMPL_JOB_ATTACH,
        make_scoped_refptr(new NetLogSourceParameter(
            "source_dependency", net_log_.source())));

    net_log_.AddEvent(
        NetLog::TYPE_HOST_RESOLVER_IMPL_JOB_REQUEST_ATTACH,
        make_scoped_refptr(new JobAttachParameters(
            req->request_net_log().source(), priority())));

    // TODO(szym): Check if this is still needed.
    if (!req->info().is_speculative()) {
      had_non_speculative_request_ = true;
      if (proc_task_)
        proc_task_->set_had_non_speculative_request();
    }
  }

  void CancelRequest(Request* req) {
    DCHECK_EQ(key_.hostname, req->info().hostname());
    DCHECK(!req->was_canceled());
    // Don't remove it from |requests_| just mark it canceled.
    req->MarkAsCanceled();
    LogCancelRequest(req->source_net_log(), req->request_net_log(),
                     req->info());
    priority_tracker_.Remove(req->info().priority());
    net_log_.AddEvent(
        NetLog::TYPE_HOST_RESOLVER_IMPL_JOB_REQUEST_DETACH,
        make_scoped_refptr(new JobAttachParameters(
            req->request_net_log().source(), priority())));
  }

  // Aborts and destroys the job, completes all requests as aborted.
  void Abort() {
    // Job should only be aborted if it's running.
    DCHECK(is_running());
    proc_task_->Cancel();
    proc_task_ = NULL;
    net_error_ = ERR_ABORTED;
    os_error_ = 0;
    CompleteRequests(AddressList());
  }

  bool is_running() const {
    return proc_task_.get() != NULL;
  }

  // Called by HostResolverImpl when this job is evicted due to queue overflow.
  void OnEvicted() {
    // Must not be running.
    DCHECK(!is_running());
    handle_ = PrioritizedDispatcher::Handle();

    net_log_.AddEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_JOB_EVICTED, NULL);

    // This signals to CompleteRequests that this job never ran.
    net_error_ = ERR_HOST_RESOLVER_QUEUE_TOO_LARGE;
    os_error_ = 0;
    CompleteRequests(AddressList());
  }

  // PriorityDispatch::Job interface.
  virtual void Start() OVERRIDE {
    DCHECK(!is_running());
    handle_ = PrioritizedDispatcher::Handle();

    net_log_.AddEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_JOB_STARTED, NULL);

    proc_task_ = new ProcTask(
        key_,
        resolver_->proc_params_,
        base::Bind(&Job::OnProcTaskComplete, base::Unretained(this)),
        net_log_);

    if (had_non_speculative_request_)
      proc_task_->set_had_non_speculative_request();
    // Start() could be called from within Resolve(), hence it must NOT directly
    // call OnProcTaskComplete, for example, on synchronous failure.
    proc_task_->Start();
  }

 private:
  // Called by ProcTask when it completes.
  void OnProcTaskComplete(int net_error, int os_error,
                          const AddressList& addrlist) {
    DCHECK(is_running());
    proc_task_ = NULL;
    net_error_ = net_error;
    os_error_ = os_error;

    // We are the only consumer of |addrlist|, so we can safely change the port
    // without copy-on-write. This pays off, when job has only one request.
    AddressList list = addrlist;
    if (net_error == OK && !requests_.empty())
      MutableSetPort(requests_.front()->info().port(), &list);
    CompleteRequests(list);
  }

  // Completes all Requests. Calls OnJobFinished and deletes self.
  void CompleteRequests(const AddressList& addrlist) {
    CHECK(resolver_);

    // This job must be removed from resolver's |jobs_| now to make room for a
    // new job with the same key in case one of the OnComplete callbacks decides
    // to spawn one. Consequently, the job deletes itself when CompleteRequests
    // is done.
    scoped_ptr<Job> self_deleter(this);
    resolver_->OnJobFinished(this, addrlist);

    // Complete all of the requests that were attached to the job.
    for (RequestsList::const_iterator it = requests_.begin();
         it != requests_.end(); ++it) {
      Request* req = *it;

      if (req->was_canceled())
        continue;

      DCHECK_EQ(this, req->job());
      // Update the net log and notify registered observers.
      LogFinishRequest(req->source_net_log(), req->request_net_log(),
                       req->info(), net_error_, os_error_);

      req->OnComplete(net_error_, addrlist);

      // Check if the resolver was destroyed as a result of running the
      // callback. If it was, we could continue, but we choose to bail.
      if (!resolver_)
        return;
    }
  }

  // Used to call OnJobFinished and RemoveJob.
  base::WeakPtr<HostResolverImpl> resolver_;

  Key key_;

  // Tracks the highest priority across |requests_|.
  PriorityTracker priority_tracker_;

  bool had_non_speculative_request_;

  BoundNetLog net_log_;

  // Store result here in case the job fails fast in Resolve().
  int net_error_;
  int os_error_;

  // A ProcTask created and started when this Job is dispatched..
  scoped_refptr<ProcTask> proc_task_;

  // All Requests waiting for the result of this Job. Some can be canceled.
  RequestsList requests_;

  // A handle used by HostResolverImpl in |dispatcher_|.
  PrioritizedDispatcher::Handle handle_;
};

//-----------------------------------------------------------------------------

HostResolverImpl::ProcTaskParams::ProcTaskParams(
    HostResolverProc* resolver_proc,
    size_t max_retry_attempts)
    : resolver_proc(resolver_proc),
      max_retry_attempts(max_retry_attempts),
      unresponsive_delay(base::TimeDelta::FromMilliseconds(6000)),
      retry_factor(2) {
}

HostResolverImpl::ProcTaskParams::~ProcTaskParams() {}

HostResolverImpl::HostResolverImpl(
    HostCache* cache,
    const PrioritizedDispatcher::Limits& job_limits,
    const ProcTaskParams& proc_params,
    NetLog* net_log)
    : cache_(cache),
      dispatcher_(job_limits),
      max_queued_jobs_(job_limits.total_jobs * 100u),
      proc_params_(proc_params),
      default_address_family_(ADDRESS_FAMILY_UNSPECIFIED),
      ipv6_probe_monitoring_(false),
      additional_resolver_flags_(0),
      net_log_(net_log) {

  DCHECK_GE(dispatcher_.num_priorities(), static_cast<size_t>(NUM_PRIORITIES));

  // Maximum of 4 retry attempts for host resolution.
  static const size_t kDefaultMaxRetryAttempts = 4u;

  if (proc_params_.max_retry_attempts == HostResolver::kDefaultRetryAttempts)
    proc_params_.max_retry_attempts = kDefaultMaxRetryAttempts;

#if defined(OS_WIN)
  EnsureWinsockInit();
#endif
#if defined(OS_POSIX) && !defined(OS_MACOSX)
  if (HaveOnlyLoopbackAddresses())
    additional_resolver_flags_ |= HOST_RESOLVER_LOOPBACK_ONLY;
#endif
  NetworkChangeNotifier::AddIPAddressObserver(this);
#if defined(OS_POSIX) && !defined(OS_MACOSX) && !defined(OS_OPENBSD)
#if !defined(OS_ANDROID)
  EnsureDnsReloaderInit();
#endif
  NetworkChangeNotifier::AddDNSObserver(this);
#endif
}

HostResolverImpl::~HostResolverImpl() {
  DiscardIPv6ProbeJob();

  // This will also cancel all outstanding requests.
  STLDeleteValues(&jobs_);

  NetworkChangeNotifier::RemoveIPAddressObserver(this);
#if defined(OS_POSIX) && !defined(OS_MACOSX) && !defined(OS_OPENBSD)
  NetworkChangeNotifier::RemoveDNSObserver(this);
#endif
}

void HostResolverImpl::SetMaxQueuedJobs(size_t value) {
  DCHECK_EQ(0u, dispatcher_.num_queued_jobs());
  DCHECK_GT(value, 0u);
  max_queued_jobs_ = value;
}

int HostResolverImpl::Resolve(const RequestInfo& info,
                              AddressList* addresses,
                              const CompletionCallback& callback,
                              RequestHandle* out_req,
                              const BoundNetLog& source_net_log) {
  DCHECK(addresses);
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(false, callback.is_null());

  // Make a log item for the request.
  BoundNetLog request_net_log = BoundNetLog::Make(net_log_,
      NetLog::SOURCE_HOST_RESOLVER_IMPL_REQUEST);

  LogStartRequest(source_net_log, request_net_log, info);

  // Build a key that identifies the request in the cache and in the
  // outstanding jobs map.
  Key key = GetEffectiveKeyForRequest(info);

  int rv = ResolveHelper(key, info, addresses, request_net_log);
  if (rv != ERR_DNS_CACHE_MISS) {
    LogFinishRequest(source_net_log, request_net_log, info, rv,
                     0  /* os_error (unknown since from cache) */);
    return rv;
  }

  // Next we need to attach our request to a "job". This job is responsible for
  // calling "getaddrinfo(hostname)" on a worker thread.

  JobMap::iterator jobit = jobs_.find(key);
  Job* job;
  if (jobit == jobs_.end()) {
    // Create new Job.
    job = new Job(this, key, request_net_log);
    job->set_handle(dispatcher_.Add(job, info.priority()));

    // Check for queue overflow.
    if (dispatcher_.num_queued_jobs() > max_queued_jobs_) {
      Job* evicted = static_cast<Job*>(dispatcher_.EvictOldestLowest());
      DCHECK(evicted);
      if (evicted == job) {
        delete job;
        rv = ERR_HOST_RESOLVER_QUEUE_TOO_LARGE;
        LogFinishRequest(source_net_log, request_net_log, info, rv, 0);
        return rv;
      }
      evicted->OnEvicted();  // Deletes |evicted|.
    }

    jobs_.insert(jobit, std::make_pair(key, job));
  } else {
    job = jobit->second;
  }

  // Can't complete synchronously. Create and attach request.
  Request* req = new Request(source_net_log, request_net_log, info, callback,
                             addresses);
  job->AddRequest(req);
  if (!job->handle().is_null())
    job->set_handle(dispatcher_.ChangePriority(job->handle(), job->priority()));
  if (out_req)
    *out_req = reinterpret_cast<RequestHandle>(req);

  DCHECK_EQ(ERR_IO_PENDING, job->net_error());
  // Completion happens during Job::CompleteRequests().
  return ERR_IO_PENDING;
}

int HostResolverImpl::ResolveHelper(const Key& key,
                                    const RequestInfo& info,
                                    AddressList* addresses,
                                    const BoundNetLog& request_net_log) {
  // The result of |getaddrinfo| for empty hosts is inconsistent across systems.
  // On Windows it gives the default interface's address, whereas on Linux it
  // gives an error. We will make it fail on all platforms for consistency.
  if (info.hostname().empty() || info.hostname().size() > kMaxHostLength)
    return ERR_NAME_NOT_RESOLVED;

  int net_error = ERR_UNEXPECTED;
  if (ResolveAsIP(key, info, &net_error, addresses))
    return net_error;
  net_error = ERR_DNS_CACHE_MISS;
  ServeFromCache(key, info, request_net_log, &net_error, addresses);
  return net_error;
}

int HostResolverImpl::ResolveFromCache(const RequestInfo& info,
                                       AddressList* addresses,
                                       const BoundNetLog& source_net_log) {
  DCHECK(CalledOnValidThread());
  DCHECK(addresses);

  // Make a log item for the request.
  BoundNetLog request_net_log = BoundNetLog::Make(net_log_,
      NetLog::SOURCE_HOST_RESOLVER_IMPL_REQUEST);

  // Update the net log and notify registered observers.
  LogStartRequest(source_net_log, request_net_log, info);

  Key key = GetEffectiveKeyForRequest(info);

  int rv = ResolveHelper(key, info, addresses, request_net_log);
  LogFinishRequest(source_net_log, request_net_log, info, rv,
                   0  /* os_error (unknown since from cache) */);
  return rv;
}

void HostResolverImpl::CancelRequest(RequestHandle req_handle) {
  DCHECK(CalledOnValidThread());
  Request* req = reinterpret_cast<Request*>(req_handle);
  DCHECK(req);

  Job* job = req->job();
  DCHECK(job);

  job->CancelRequest(req);

  if (!job->handle().is_null()) {
    // Still in queue.
    if (job->num_active_requests()) {
      job->set_handle(dispatcher_.ChangePriority(job->handle(),
                                                 job->priority()));
    } else {
      dispatcher_.Cancel(job->handle());
      RemoveJob(job);
      delete job;
    }
  } else {
    // Job is running (and could be in CompleteRequests right now).
    // But to be in Request::OnComplete we would have to have a non-canceled
    // request. So it is safe to Abort it if it has no more active requests.
    if (!job->num_active_requests()) {
      job->Abort();
    }
  }
}

void HostResolverImpl::SetDefaultAddressFamily(AddressFamily address_family) {
  DCHECK(CalledOnValidThread());
  ipv6_probe_monitoring_ = false;
  DiscardIPv6ProbeJob();
  default_address_family_ = address_family;
}

AddressFamily HostResolverImpl::GetDefaultAddressFamily() const {
  return default_address_family_;
}

void HostResolverImpl::ProbeIPv6Support() {
  DCHECK(CalledOnValidThread());
  DCHECK(!ipv6_probe_monitoring_);
  ipv6_probe_monitoring_ = true;
  OnIPAddressChanged();  // Give initial setup call.
}

HostCache* HostResolverImpl::GetHostCache() {
  return cache_.get();
}

bool HostResolverImpl::ResolveAsIP(const Key& key,
                                   const RequestInfo& info,
                                   int* net_error,
                                   AddressList* addresses) {
  DCHECK(addresses);
  DCHECK(net_error);
  IPAddressNumber ip_number;
  if (!ParseIPLiteralToNumber(key.hostname, &ip_number))
    return false;

  DCHECK_EQ(key.host_resolver_flags &
      ~(HOST_RESOLVER_CANONNAME | HOST_RESOLVER_LOOPBACK_ONLY |
        HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6),
            0) << " Unhandled flag";
  bool ipv6_disabled = (default_address_family_ == ADDRESS_FAMILY_IPV4) &&
      !ipv6_probe_monitoring_;
  *net_error = OK;
  if ((ip_number.size() == kIPv6AddressSize) && ipv6_disabled) {
    *net_error = ERR_NAME_NOT_RESOLVED;
  } else {
    *addresses = AddressList::CreateFromIPAddressWithCname(
        ip_number, info.port(),
        (key.host_resolver_flags & HOST_RESOLVER_CANONNAME));
  }
  return true;
}

bool HostResolverImpl::ServeFromCache(const Key& key,
                                      const RequestInfo& info,
                                      const BoundNetLog& request_net_log,
                                      int* net_error,
                                      AddressList* addresses) {
  DCHECK(addresses);
  DCHECK(net_error);
  if (!info.allow_cached_response() || !cache_.get())
    return false;

  const HostCache::Entry* cache_entry = cache_->Lookup(
      key, base::TimeTicks::Now());
  if (!cache_entry)
    return false;

  request_net_log.AddEvent(NetLog::TYPE_HOST_RESOLVER_IMPL_CACHE_HIT, NULL);
  *net_error = cache_entry->error;
  if (*net_error == OK)
    *addresses = CreateAddressListUsingPort(cache_entry->addrlist, info.port());
  return true;
}

void HostResolverImpl::OnJobFinished(Job* job, const AddressList& addrlist) {
  DCHECK(job);
  DCHECK(job->handle().is_null());
  RemoveJob(job);
  if (job->net_error() == ERR_HOST_RESOLVER_QUEUE_TOO_LARGE)
    return;

  // Signal dispatcher that a slot has opened.
  dispatcher_.OnJobFinished();
  if (job->net_error() == ERR_ABORTED)
    return;
  // Write result to the cache.
  if (cache_.get()) {
    base::TimeDelta ttl = base::TimeDelta::FromSeconds(0);
    if (job->net_error() == OK)
      ttl = base::TimeDelta::FromSeconds(kCacheEntryTTLSeconds);
    cache_->Set(job->key(), job->net_error(), addrlist,
                base::TimeTicks::Now(), ttl);
  }
}

void HostResolverImpl::RemoveJob(Job* job) {
  DCHECK(job);
  jobs_.erase(job->key());
}

void HostResolverImpl::DiscardIPv6ProbeJob() {
  if (ipv6_probe_job_.get()) {
    ipv6_probe_job_->Cancel();
    ipv6_probe_job_ = NULL;
  }
}

void HostResolverImpl::IPv6ProbeSetDefaultAddressFamily(
    AddressFamily address_family) {
  DCHECK(address_family == ADDRESS_FAMILY_UNSPECIFIED ||
         address_family == ADDRESS_FAMILY_IPV4);
  if (default_address_family_ != address_family) {
    VLOG(1) << "IPv6Probe forced AddressFamily setting to "
            << ((address_family == ADDRESS_FAMILY_UNSPECIFIED) ?
                "ADDRESS_FAMILY_UNSPECIFIED" : "ADDRESS_FAMILY_IPV4");
  }
  default_address_family_ = address_family;
  // Drop reference since the job has called us back.
  DiscardIPv6ProbeJob();
}

HostResolverImpl::Key HostResolverImpl::GetEffectiveKeyForRequest(
    const RequestInfo& info) const {
  HostResolverFlags effective_flags =
      info.host_resolver_flags() | additional_resolver_flags_;
  AddressFamily effective_address_family = info.address_family();
  if (effective_address_family == ADDRESS_FAMILY_UNSPECIFIED &&
      default_address_family_ != ADDRESS_FAMILY_UNSPECIFIED) {
    effective_address_family = default_address_family_;
    if (ipv6_probe_monitoring_)
      effective_flags |= HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6;
  }
  return Key(info.hostname(), effective_address_family, effective_flags);
}

void HostResolverImpl::AbortAllInProgressJobs() {
  base::WeakPtr<HostResolverImpl> self = AsWeakPtr();
  // Scan |jobs_| for running jobs and abort them.
  for (JobMap::iterator it = jobs_.begin(); it != jobs_.end(); ) {
    Job* job = it->second;
    // Advance the iterator before we might erase it.
    ++it;
    if (job->is_running()) {
      job->Abort();
      // Check if resolver was deleted in a request callback.
      if (!self)
        return;
    } else {
      // Keep it in |dispatch_|.
      DCHECK(!job->handle().is_null());
    }
  }
}

void HostResolverImpl::OnIPAddressChanged() {
  if (cache_.get())
    cache_->clear();
  if (ipv6_probe_monitoring_) {
    DiscardIPv6ProbeJob();
    ipv6_probe_job_ = new IPv6ProbeJob(this);
    ipv6_probe_job_->Start();
  }
#if defined(OS_POSIX) && !defined(OS_MACOSX)
  if (HaveOnlyLoopbackAddresses()) {
    additional_resolver_flags_ |= HOST_RESOLVER_LOOPBACK_ONLY;
  } else {
    additional_resolver_flags_ &= ~HOST_RESOLVER_LOOPBACK_ONLY;
  }
#endif
  AbortAllInProgressJobs();
  // |this| may be deleted inside AbortAllInProgressJobs().
}

void HostResolverImpl::OnDNSChanged() {
  // If the DNS server has changed, existing cached info could be wrong so we
  // have to drop our internal cache :( Note that OS level DNS caches, such
  // as NSCD's cache should be dropped automatically by the OS when
  // resolv.conf changes so we don't need to do anything to clear that cache.
  if (cache_.get())
    cache_->clear();
  // Existing jobs will have been sent to the original server so they need to
  // be aborted. TODO(Craig): Should these jobs be restarted?
  AbortAllInProgressJobs();
  // |this| may be deleted inside AbortAllInProgressJobs().
}

}  // namespace net
