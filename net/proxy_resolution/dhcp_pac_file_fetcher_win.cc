// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy_resolution/dhcp_pac_file_fetcher_win.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/free_deleter.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/values.h"
#include "net/base/net_errors.h"
#include "net/log/net_log.h"
#include "net/proxy_resolution/dhcp_pac_file_adapter_fetcher_win.h"

#include <winsock2.h>
#include <iphlpapi.h>

namespace net {

namespace {

// How many threads to use at maximum to do DHCP lookups. This is
// chosen based on the following UMA data:
// - When OnWaitTimer fires, ~99.8% of users have 6 or fewer network
//   adapters enabled for DHCP in total.
// - At the same measurement point, ~99.7% of users have 3 or fewer pending
//   DHCP adapter lookups.
// - There is however a very long and thin tail of users who have
//   systems reporting up to 100+ adapters (this must be some very weird
//   OS bug (?), probably the cause of http://crbug.com/240034).
//
// The maximum number of threads is chosen such that even systems that
// report a huge number of network adapters should not run out of
// memory from this number of threads, while giving a good chance of
// getting back results for any responsive adapters.
//
// The ~99.8% of systems that have 6 or fewer network adapters will
// not grow the thread pool to its maximum size (rather, they will
// grow it to 6 or fewer threads) so setting the limit lower would not
// improve performance or memory usage on those systems.
const int kMaxDhcpLookupThreads = 12;

// How long to wait at maximum after we get results (a PAC file or
// knowledge that no PAC file is configured) from whichever network
// adapter finishes first.
const int kMaxWaitAfterFirstResultMs = 400;

std::unique_ptr<base::Value> NetLogGetAdaptersDoneCallback(DhcpAdapterNamesLoggingInfo* info, NetLogCaptureMode /* capture_mode */) {
	std::unique_ptr<base::DictionaryValue> result =
		std::make_unique<base::DictionaryValue>();

	IP_ADAPTER_ADDRESSES* adapter = adapters.get();

	// Add information on each of the adapters enumerated (including those that were subsequently skipped).
	base::ListValue adapters_value;
	for (; adapter; adapter = adapter->Next) {
		base::DictionaryValue adapter_value;

		adapter_value.SetString("AdapterName", adapter->AdapterName);
		adapter_value.SetInteger("IfType", adapter->IfType);
		adapter_value.SetInteger("Flags", adapter->Flags);
		adapter_value.SetInteger("OperStatus", adapter->OperStatus);
		adapter_value.SetInteger("TunnelType", adapter->TunnelType);

		// "skipped" means the adapter was not ultimately chosen as a candidate for
		// testing WPAD. This replicates the logic in GetAdapterNames().
		bool skipped = (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) ||
			((adapter->Flags & IP_ADAPTER_DHCP_ENABLED) == 0);
		adapter_value.SetBoolean("skipped", skipped);

		adapters_value.GetList().push_back(std::move(adapter_value));
	}
	result->SetKey("adapters", std::move(adapters_value));

	result->SetInteger("start_wait_ms", (worker_thread_start_time - origin_thread_start_time).InMilliseconds());
	result->SetInteger("end_wait_ms", (origin_thread_end_time - worker_thread_end_time).InMilliseconds());
	result->SetInteger("duration_ms", (worker_thread_end_time - worker_thread_end_time).InMilliseconds());

	if (error != ERROR_SUCCESS)
		result->SetInteger("error", error);

	return result;
}

std::unique_ptr<base::Value> NetLogFetcherDoneCallback(int fetcher_i, int net_error, NetLogCaptureMode /* capture_mode */) {
	std::unique_ptr<base::DictionaryValue> result =
		std::make_unique<base::DictionaryValue>();

  result->SetInteger("fetcher_i", fetcher_i);
  result->SetInteger("net_error", net_error);

  return result;
}

}  // namespace

DhcpAdapterNamesLoggingInfo::DhcpAdapterNamesLoggingInfo() = default;
DhcpAdapterNamesLoggingInfo::~DhcpAdapterNamesLoggingInfo() = default;

DhcpProxyScriptFetcherWin::DhcpProxyScriptFetcherWin(
    URLRequestContext* url_request_context)
    : state_(STATE_START),
      num_pending_fetchers_(0),
      destination_string_(NULL),
      url_request_context_(url_request_context) {
  DCHECK(url_request_context_);

  worker_pool_ = new base::SequencedWorkerPool(
      kMaxDhcpLookupThreads, "PacDhcpLookup", base::TaskPriority::USER_VISIBLE);
}

DhcpProxyScriptFetcherWin::~DhcpProxyScriptFetcherWin() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Count as user-initiated if we are not yet in STATE_DONE.
  Cancel();

  worker_pool_->Shutdown();
}

int DhcpProxyScriptFetcherWin::Fetch(base::string16* utf16_text,
                                     const CompletionCallback& callback,
                                     const NetLogWithSource& net_log) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (state_ != STATE_START && state_ != STATE_DONE) {
    NOTREACHED();
    return ERR_UNEXPECTED;
  }

  net_log_ = net_log;

  if (!url_request_context_)
    return ERR_CONTEXT_SHUT_DOWN;

  state_ = STATE_WAIT_ADAPTERS;
  callback_ = callback;
  destination_string_ = utf16_text;

  net_log.BeginEvent(NetLogEventType::WPAD_DHCP_WIN_FETCH);
  net_log.BeginEvent(NetLogEventType::WPAD_DHCP_WIN_GET_ADAPTERS);

  last_query_ = ImplCreateAdapterQuery();

  last_query_->logging_info()->origin_thread_start_time = base::TimeTicks();

  GetTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(
          &DhcpProxyScriptFetcherWin::AdapterQuery::GetCandidateAdapterNames,
          last_query_.get()),
      base::Bind(
          &DhcpProxyScriptFetcherWin::OnGetCandidateAdapterNamesDone,
          AsWeakPtr(),
          last_query_));

  return ERR_IO_PENDING;
}

void DhcpProxyScriptFetcherWin::Cancel() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  CancelImpl();
}

void DhcpProxyScriptFetcherWin::OnShutdown() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Back up callback, if there is one, as CancelImpl() will destroy it.
  net::CompletionCallback callback = std::move(callback_);

  // Cancel current request, if there is one.
  CancelImpl();

  // Prevent future network requests.
  url_request_context_ = nullptr;

  // Invoke callback with error, if present.
  if (callback)
    callback.Run(ERR_CONTEXT_SHUT_DOWN);
}

void DhcpProxyScriptFetcherWin::CancelImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (state_ != STATE_DONE) {
    callback_.Reset();
    wait_timer_.Stop();
    state_ = STATE_DONE;

    for (FetcherVector::iterator it = fetchers_.begin();
         it != fetchers_.end();
         ++it) {
      (*it)->Cancel();
    }

    fetchers_.clear();
  }
}

void DhcpProxyScriptFetcherWin::OnGetCandidateAdapterNamesDone(
    scoped_refptr<AdapterQuery> query) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // This can happen if this object is reused for multiple queries,
  // and a previous query was cancelled before it completed.
  if (query.get() != last_query_.get())
    return;
  last_query_ = NULL;

  query->logging_info()->origin_thread_end_time = base::TimeTicks();
  net_log_.EndEvent(
      NetLogEventType::WPAD_DHCP_WIN_GET_ADAPTERS,
      base::Bind(&NetLogGetAdaptersDoneCallback, base::Unretained(query->logging_info())));

  // Enable unit tests to wait for this to happen; in production this function
  // call is a no-op.
  ImplOnGetCandidateAdapterNamesDone();

  // We may have been cancelled.
  if (state_ != STATE_WAIT_ADAPTERS)
    return;

  state_ = STATE_NO_RESULTS;

  const std::set<std::string>& adapter_names = query->adapter_names();

  if (adapter_names.empty()) {
    TransitionToDone();
    return;
  }

  for (const std::string& adapter_name : adapter_names) {
    std::unique_ptr<DhcpProxyScriptAdapterFetcher> fetcher(ImplCreateAdapterFetcher());
    size_t fetcher_i = fetchers_.size();
    fetcher->Fetch(adapter_name,
                   base::Bind(&DhcpProxyScriptFetcherWin::OnFetcherDone,
                              base::Unretained(this), fetcher_i),
                   net_log_);
    fetchers_.push_back(std::move(fetcher));
  }
  num_pending_fetchers_ = fetchers_.size();
}

std::string DhcpProxyScriptFetcherWin::GetFetcherName() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return "win";
}

const GURL& DhcpProxyScriptFetcherWin::GetPacURL() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(state_, STATE_DONE);

  return pac_url_;
}

void DhcpProxyScriptFetcherWin::OnFetcherDone(size_t fetcher_i, int result) {
  DCHECK(state_ == STATE_NO_RESULTS || state_ == STATE_SOME_RESULTS);

  net_log_.AddEvent(NetLogEventType::WPAD_DHCP_WIN_ON_FETCHER_DONE,
                    base::Bind(&NetLogFetcherDoneCallback, fetcher_i, result));

  if (--num_pending_fetchers_ == 0) {
    TransitionToDone();
    return;
  }

  // If the only pending adapters are those less preferred than one
  // with a valid PAC script, we do not need to wait any longer.
  for (FetcherVector::iterator it = fetchers_.begin();
       it != fetchers_.end();
       ++it) {
    bool did_finish = (*it)->DidFinish();
    int result = (*it)->GetResult();
    if (did_finish && result == OK) {
      TransitionToDone();
      return;
    }
    if (!did_finish || result != ERR_PAC_NOT_IN_DHCP) {
      break;
    }
  }

  // Once we have a single result, we set a maximum on how long to wait
  // for the rest of the results.
  if (state_ == STATE_NO_RESULTS) {
    state_ = STATE_SOME_RESULTS;
    net_log_.AddEvent(NetLogEventType::WPAD_DHCP_WIN_START_WAIT_TIMER);
    wait_timer_.Start(FROM_HERE,
        ImplGetMaxWait(), this, &DhcpProxyScriptFetcherWin::OnWaitTimer);
  }
}

void DhcpProxyScriptFetcherWin::OnWaitTimer() {
  DCHECK_EQ(state_, STATE_SOME_RESULTS);

  net_log_.AddEvent(NetLogEventType::WPAD_DHCP_WIN_ON_WAIT_TIMER);
  TransitionToDone();
}

void DhcpProxyScriptFetcherWin::TransitionToDone() {
  DCHECK(state_ == STATE_NO_RESULTS || state_ == STATE_SOME_RESULTS);

  int fetcher_i = -1;
  int result = ERR_PAC_NOT_IN_DHCP;  // Default if no fetchers.
  if (!fetchers_.empty()) {
    // Scan twice for the result; once through the whole list for success,
    // then if no success, return result for most preferred network adapter,
    // preferring "real" network errors to the ERR_PAC_NOT_IN_DHCP error.
    // Default to ERR_ABORTED if no fetcher completed.
    result = ERR_ABORTED;
    for (size_t i = 0; i < fetchers_.size(); ++i) {
      const auto& fetcher = fetchers_[i];
      if (fetcher->DidFinish() && fetcher->GetResult() == OK) {
        result = OK;
        *destination_string_ = fetcher->GetPacScript();
        pac_url_ = fetcher->GetPacURL();
        fetcher_i = i;
        break;
      }
    }
    if (result != OK) {
      destination_string_->clear();
      for (size_t i = 0; i < fetchers_.size(); ++i) {
        const auto& fetcher = fetchers_[i];
        if (fetcher->DidFinish()) {
          result = fetcher->GetResult();
          fetcher_i = i;
          if (result != ERR_PAC_NOT_IN_DHCP) {
            break;
          }
        }
      }
    }
  }

  CompletionCallback callback = callback_;
  CancelImpl();
  DCHECK_EQ(state_, STATE_DONE);
  DCHECK(fetchers_.empty());
  DCHECK(callback_.is_null());  // Invariant of data.

  net_log_.EndEvent(NetLogEventType::WPAD_DHCP_WIN_FETCH,
      base::Bind(&NetLogFetcherDoneCallback, fetcher_i, result));

  // We may be deleted re-entrantly within this outcall.
  callback.Run(result);
}

int DhcpProxyScriptFetcherWin::num_pending_fetchers() const {
  return num_pending_fetchers_;
}

URLRequestContext* DhcpProxyScriptFetcherWin::url_request_context() const {
  return url_request_context_;
}

scoped_refptr<base::TaskRunner> DhcpProxyScriptFetcherWin::GetTaskRunner() {
  return worker_pool_->GetTaskRunnerWithShutdownBehavior(
      base::SequencedWorkerPool::CONTINUE_ON_SHUTDOWN);
}

DhcpProxyScriptAdapterFetcher*
    DhcpProxyScriptFetcherWin::ImplCreateAdapterFetcher() {
  return new DhcpProxyScriptAdapterFetcher(url_request_context_,
                                           GetTaskRunner());
}

DhcpProxyScriptFetcherWin::AdapterQuery*
    DhcpProxyScriptFetcherWin::ImplCreateAdapterQuery() {
  return new AdapterQuery();
}

base::TimeDelta DhcpProxyScriptFetcherWin::ImplGetMaxWait() {
  return base::TimeDelta::FromMilliseconds(kMaxWaitAfterFirstResultMs);
}

bool DhcpProxyScriptFetcherWin::GetCandidateAdapterNames(
    std::set<std::string>* adapter_names, DhcpAdapterNamesLoggingInfo* info) {
  DCHECK(adapter_names);
  adapter_names->clear();

  // The GetAdaptersAddresses MSDN page recommends using a size of 15000 to
  // avoid reallocation.
  ULONG adapters_size = 15000;
  std::unique_ptr<IP_ADAPTER_ADDRESSES, base::FreeDeleter> adapters;
  ULONG error = ERROR_SUCCESS;
  int num_tries = 0;

  do {
    adapters.reset(static_cast<IP_ADAPTER_ADDRESSES*>(malloc(adapters_size)));
    // Return only unicast addresses, and skip information we do not need.
    error = GetAdaptersAddresses(AF_UNSPEC,
                                 GAA_FLAG_SKIP_ANYCAST |
                                 GAA_FLAG_SKIP_MULTICAST |
                                 GAA_FLAG_SKIP_DNS_SERVER |
                                 GAA_FLAG_SKIP_FRIENDLY_NAME,
                                 NULL,
                                 adapters.get(),
                                 &adapters_size);
    ++num_tries;
  } while (error == ERROR_BUFFER_OVERFLOW && num_tries <= 3);

  info->error = error;

  if (error == ERROR_NO_DATA) {
    // There are no adapters that we care about.
    return true;
  }

  if (error != ERROR_SUCCESS) {
    LOG(WARNING) << "Unexpected error retrieving WPAD configuration from DHCP.";
    return false;
  }

  IP_ADAPTER_ADDRESSES* adapter = NULL;
  for (adapter = adapters.get(); adapter; adapter = adapter->Next) {
    if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
      continue;
    if ((adapter->Flags & IP_ADAPTER_DHCP_ENABLED) == 0)
      continue;

    DCHECK(adapter->AdapterName);
    adapter_names->insert(adapter->AdapterName);
  }

  // Transfer the buffer containing the adapters, so it can be used later for
  // emitting NetLog parameters from the origin thread.
  info->adapters = std::move(adapters);
  return true;
}

DhcpProxyScriptFetcherWin::AdapterQuery::AdapterQuery() {
}

void DhcpProxyScriptFetcherWin::AdapterQuery::GetCandidateAdapterNames() {
  logging_info_.error = ERROR_NO_DATA;
  logging_info_.adapters.reset();
  logging_info_.worker_thread_start_time = base::TimeTicks::Now();

  ImplGetCandidateAdapterNames(&adapter_names_, &logging_info_);

  logging_info_.worker_thread_end_time = base::TimeTicks::Now();
}

const std::set<std::string>&
    DhcpProxyScriptFetcherWin::AdapterQuery::adapter_names() const {
  return adapter_names_;
}

DhcpAdapterNamesLoggingInfo*
DhcpProxyScriptFetcherWin::AdapterQuery::logging_info() {
	return &logging_info_;
}


bool DhcpProxyScriptFetcherWin::AdapterQuery::ImplGetCandidateAdapterNames(
    std::set<std::string>* adapter_names, DhcpAdapterNamesLoggingInfo* info) {
  return DhcpProxyScriptFetcherWin::GetCandidateAdapterNames(adapter_names, info);
}

DhcpProxyScriptFetcherWin::AdapterQuery::~AdapterQuery() {
}

}  // namespace net
