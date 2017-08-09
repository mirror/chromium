// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_cache_writers.h"

#include <algorithm>
#include <utility>

#include "base/auto_reset.h"
#include "base/logging.h"

#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache_transaction.h"

namespace net {

HttpCache::Writers::Writers(HttpCache* cache, HttpCache::ActiveEntry* entry)
    : cache_(cache), entry_(entry), weak_factory_(this) {}

HttpCache::Writers::~Writers() {}

int HttpCache::Writers::Read(scoped_refptr<IOBuffer> buf,
                             int buf_len,
                             const CompletionCallback& callback,
                             Transaction* transaction) {
  DCHECK(buf);
  DCHECK_GT(buf_len, 0);
  DCHECK(!callback.is_null());
  DCHECK(transaction);

  // If another transaction invoked a Read which is currently ongoing, then
  // this transaction waits for the read to complete and gets its buffer filled
  // with the data returned from that read.
  if (next_state_ != State::NONE) {
    WaitingForRead waiting_transaction(transaction, buf, buf_len, callback);
    waiting_for_read_.push_back(waiting_transaction);
    return ERR_IO_PENDING;
  }

  DCHECK_EQ(next_state_, State::NONE);
  DCHECK(callback_.is_null());
  DCHECK_EQ(nullptr, active_transaction_);
  DCHECK(HasTransaction(transaction));
  active_transaction_ = transaction;

  read_buf_ = std::move(buf);
  io_buf_len_ = buf_len;
  next_state_ = State::NETWORK_READ;

  int rv = DoLoop(OK);
  if (rv == ERR_IO_PENDING)
    callback_ = callback;

  return rv;
}

bool HttpCache::Writers::StopCaching(Transaction* transaction) {
  // If this is the only transaction in Writers, then stopping will be
  // successful. If not, then we will not stop caching since there are
  // other consumers waiting to read from the cache.
  if (all_writers_.size() == 1) {
    DCHECK(all_writers_.count(transaction));
    network_read_only_ = true;
    // Let entry_ know so that any dependent transactions are restarted and
    // this entry is doomed.
    keep_entry_ = false;
    cache_->DoneWithEntryWriters(entry_, false /* success */, nullptr);
    return true;
  }
  return false;
}

void HttpCache::Writers::DoneReading(Transaction* transaction) {
  // Should not be invoked when this transaction is reading.
  DCHECK(!active_transaction_ || active_transaction_ != transaction);

  // If another transaction is currently reading, let it continue, just remove
  // this transaction.
  if (active_transaction_ && active_transaction_ != transaction) {
    RemoveTransaction(transaction);
    return;
  }

  // Invoke entry processing. Note it will also remove transaction
  // from |this|.
  cache_->DoneWithEntryWriters(entry_, true /* success */, transaction);
}

void HttpCache::Writers::AddTransaction(Transaction* transaction,
                                        bool is_exclusive) {
  DCHECK(transaction);
  DCHECK(CanAddWriters());

  std::pair<TransactionSet::iterator, bool> return_val =
      all_writers_.insert(transaction);
  DCHECK_EQ(return_val.second, true);

  if (is_exclusive) {
    DCHECK_EQ(1u, all_writers_.size());
    is_exclusive_ = true;
  }

  if (transaction->mode() == Transaction::NONE) {
    DCHECK_EQ(1u, all_writers_.size());
    DCHECK(is_exclusive_);
    network_read_only_ = true;
  }

  priority_ = std::max(transaction->priority(), priority_);
  if (network_transaction_) {
    network_transaction_->SetPriority(priority_);
  }
}

void HttpCache::Writers::SetNetworkTransaction(
    Transaction* transaction,
    std::unique_ptr<HttpTransaction> network_transaction) {
  DCHECK_EQ(1u, all_writers_.count(transaction));
  DCHECK(network_transaction);
  network_transaction_ = std::move(network_transaction);
  network_transaction_->SetPriority(priority_);
}

void HttpCache::Writers::ResetNetworkTransaction(Transaction* transaction) {
  DCHECK(is_exclusive_);
  DCHECK_EQ(1u, all_writers_.size());
  DCHECK_EQ(transaction, *(all_writers_.begin()));
  DCHECK(transaction->partial());
  transaction->SaveWritersNetworkTransactionInfo();
  network_transaction_.reset();
}

void HttpCache::Writers::SetNetworkReadOnly(Transaction* transaction,
                                            bool success) {
  DCHECK(all_writers_.count(transaction));
  if (all_writers_.size() > 1u)
    return;
  network_read_only_ = true;
  if (!success)
    keep_entry_ = false;  // so that the entry is doomed.
  cache_->DoneWithEntryWriters(entry_, success, nullptr);
}

void HttpCache::Writers::RemoveTransaction(Transaction* transaction) {
  if (!transaction)
    return;

  if (network_transaction_)
    transaction->SaveWritersNetworkTransactionInfo();

  // The transaction should be part of all_writers.
  auto it = all_writers_.find(transaction);
  DCHECK(it != all_writers_.end());
  all_writers_.erase(it);

  if (all_writers_.empty() &&
      (next_state_ == State::NONE ||
       next_state_ == State::CACHE_WRITE_TRUNCATED_RESPONSE_COMPLETE)) {
    ResetStateForEmptyWriters();
  } else {
    UpdatePriority();
  }

  if (active_transaction_ == transaction) {
    active_transaction_ = nullptr;
    // Do not reset the callback if we are here as part of do loop processing,
    // because callback needs to be invoked at do loop exit. If not part of do
    // loop processing, reset the callback because the transaction is getting
    // destroyed.
    if (!in_do_loop_)
      callback_.Reset();
    return;
  }

  auto waiting_it = waiting_for_read_.begin();
  for (; waiting_it != waiting_for_read_.end(); waiting_it++) {
    if (transaction == waiting_it->transaction) {
      waiting_for_read_.erase(waiting_it);
      // If a waiting transaction existed, there should have been an
      // active_transaction_.
      DCHECK(active_transaction_);
      return;
    }
  }
}

void HttpCache::Writers::UpdatePriority() {
  // Get the current highest priority.
  RequestPriority current_highest = MINIMUM_PRIORITY;
  for (auto* transaction : all_writers_)
    current_highest = std::max(transaction->priority(), current_highest);

  if (priority_ != current_highest) {
    if (network_transaction_)
      network_transaction_->SetPriority(current_highest);
    priority_ = current_highest;
  }
}

bool HttpCache::Writers::ContainsOnlyIdleWriters() const {
  return waiting_for_read_.empty() && !active_transaction_;
}

HttpCache::TransactionSet HttpCache::Writers::RemoveAllIdleWriters() {
  // Should be invoked after |waiting_for_read_| transactions and
  // |active_transaction_| are processed so that all_writers_ only contains idle
  // writers.
  DCHECK(ContainsOnlyIdleWriters());

  TransactionSet idle_writers;
  idle_writers.insert(all_writers_.begin(), all_writers_.end());
  all_writers_.clear();
  ResetStateForEmptyWriters();
  return idle_writers;
}

bool HttpCache::Writers::CanAddWriters() {
  if (all_writers_.empty())
    return true;

  return !is_exclusive_ && !network_read_only_;
}

void HttpCache::Writers::ProcessFailure(Transaction* transaction, int error) {
  DCHECK(!transaction || transaction == active_transaction_);

  // Notify waiting_for_read_ of the failure. Tasks will be posted for all the
  // transactions.
  ProcessWaitingForReadTransactions(error);

  // Idle readers should fail when Read is invoked on them.
  SetIdleWritersFailState(error);
}

bool HttpCache::Writers::TruncateEntry(Transaction* transaction) {
  DCHECK_LT(0u, all_writers_.count(transaction));

  // If there are more transactions, then there is no need to truncate.
  if (all_writers_.size() > 1) {
    truncate_result_for_testing_ =
        TruncateResultForTesting::NO_OP_MORE_TRANSACTIONS;
    keep_entry_ = true;
    return true;
  }

  bool rv = TruncateEntryInternal();
  if (next_state_ == State::CACHE_WRITE_TRUNCATED_RESPONSE) {
    DoLoop(OK);
    return true;
  }
  return rv;
}

bool HttpCache::Writers::TruncateEntryInternal() {
  keep_entry_ = true;
  if (network_transaction_)
    response_info_truncation_ = network_transaction_->GetResponseInfo();

  // Don't set the flag for sparse entries.
  if (is_exclusive_) {
    DCHECK(all_writers_.size() <= 1u);
    if (all_writers_.size() == 1u) {
      auto* transaction = *(all_writers_.begin());
      if (transaction->partial()) {
        if (!transaction->truncated())
          return true;
        response_info_truncation_ = transaction->GetResponseInfo();
      }
    }
  }

  if (!CanResume()) {
    truncate_result_for_testing_ = TruncateResultForTesting::FAIL_CANNOT_RESUME;
    keep_entry_ = false;
    return false;
  }

  int current_size = entry_->disk_entry->GetDataSize(kResponseContentIndex);
  int64_t body_size = response_info_truncation_->headers->GetContentLength();
  if (body_size >= 0 && body_size <= current_size)
    return true;

  next_state_ = State::CACHE_WRITE_TRUNCATED_RESPONSE;
  return true;
}

bool HttpCache::Writers::CanResume() const {
  // Double check that there is something worth keeping.
  if (!entry_->disk_entry->GetDataSize(kResponseContentIndex))
    return false;

  // Note that if this is a 206, content-length was already fixed after calling
  // PartialData::ResponseHeadersOK().
  if (response_info_truncation_->headers->GetContentLength() <= 0 ||
      response_info_truncation_->headers->HasHeaderValue("Accept-Ranges",
                                                         "none") ||
      !response_info_truncation_->headers->HasStrongValidators()) {
    return false;
  }

  return true;
}

LoadState HttpCache::Writers::GetWriterLoadState() {
  if (network_transaction_)
    return network_transaction_->GetLoadState();
  return LOAD_STATE_IDLE;
}

HttpCache::Writers::WaitingForRead::WaitingForRead(
    Transaction* cache_transaction,
    scoped_refptr<IOBuffer> buf,
    int len,
    const CompletionCallback& consumer_callback)
    : transaction(cache_transaction),
      read_buf(std::move(buf)),
      read_buf_len(len),
      write_len(0),
      callback(consumer_callback) {
  DCHECK(cache_transaction);
  DCHECK(read_buf);
  DCHECK_GT(len, 0);
  DCHECK(!consumer_callback.is_null());
}

HttpCache::Writers::WaitingForRead::~WaitingForRead() {}
HttpCache::Writers::WaitingForRead::WaitingForRead(const WaitingForRead&) =
    default;

int HttpCache::Writers::DoLoop(int result) {
  DCHECK_NE(State::UNSET, next_state_);
  DCHECK_NE(State::NONE, next_state_);
  DCHECK(!in_do_loop_);

  int rv = result;
  do {
    State state = next_state_;
    next_state_ = State::UNSET;
    base::AutoReset<bool> scoped_in_do_loop(&in_do_loop_, true);
    switch (state) {
      case State::NETWORK_READ:
        DCHECK_EQ(OK, rv);
        rv = DoNetworkRead();
        break;
      case State::NETWORK_READ_COMPLETE:
        rv = DoNetworkReadComplete(rv);
        break;
      case State::CACHE_WRITE_DATA:
        rv = DoCacheWriteData(rv);
        break;
      case State::CACHE_WRITE_DATA_COMPLETE:
        rv = DoCacheWriteDataComplete(rv);
        break;
      case State::CACHE_WRITE_TRUNCATED_RESPONSE:
        rv = DoCacheWriteTruncatedResponse();
        break;
      case State::CACHE_WRITE_TRUNCATED_RESPONSE_COMPLETE:
        rv = DoCacheWriteTruncatedResponseComplete(rv);
        break;
      case State::UNSET:
        NOTREACHED() << "bad state";
        rv = ERR_FAILED;
        break;
      case State::NONE:
        // Do Nothing.
        break;
    }
  } while (next_state_ != State::NONE && rv != ERR_IO_PENDING);

  // Save the callback as this object may be destroyed in the cache callback.
  CompletionCallback callback = callback_;

  if (next_state_ == State::NONE) {
    read_buf_ = NULL;
    callback_.Reset();
    if (all_writers_.empty())
      ResetStateForEmptyWriters();
    if (cache_callback_)
      std::move(cache_callback_).Run();
  }

  // This object may have been destroyed in the cache_callback_.
  if (rv != ERR_IO_PENDING && !callback.is_null()) {
    base::ResetAndReturn(&callback).Run(rv);
  }
  return rv;
}

int HttpCache::Writers::DoNetworkRead() {
  DCHECK(network_transaction_);
  next_state_ = State::NETWORK_READ_COMPLETE;
  CompletionCallback io_callback =
      base::Bind(&HttpCache::Writers::OnIOComplete, weak_factory_.GetWeakPtr());
  return network_transaction_->Read(read_buf_.get(), io_buf_len_, io_callback);
}

int HttpCache::Writers::DoNetworkReadComplete(int result) {
  if (result < 0) {
    next_state_ = State::NONE;
    OnNetworkReadFailure(result);
    return result;
  }

  next_state_ = State::CACHE_WRITE_DATA;
  return result;
}

void HttpCache::Writers::OnNetworkReadFailure(int result) {
  ProcessFailure(active_transaction_, result);

  TruncateEntryInternal();

  cache_callback_ =
      base::BindOnce(&HttpCache::DoneWithEntryWriters, cache_->GetWeakPtr(),
                     entry_, false, active_transaction_);

  active_transaction_ = nullptr;
}

int HttpCache::Writers::DoCacheWriteData(int num_bytes) {
  next_state_ = State::CACHE_WRITE_DATA_COMPLETE;
  write_len_ = num_bytes;
  if (!num_bytes || network_read_only_)
    return num_bytes;

  int current_size = entry_->disk_entry->GetDataSize(kResponseContentIndex);
  CompletionCallback io_callback =
      base::Bind(&HttpCache::Writers::OnIOComplete, weak_factory_.GetWeakPtr());

  int rv = 0;

  PartialData* partial = nullptr;
  // The active transaction must be alive if this is a partial request, as
  // partial requests are exclusive and hence will always be the active
  // transaction.
  // TODO(shivanisha): When partial requests support parallel writing, this
  // assumption will not be true.
  if (active_transaction_)
    partial = active_transaction_->partial();

  if (!partial) {
    rv = entry_->disk_entry->WriteData(kResponseContentIndex, current_size,
                                       read_buf_.get(), num_bytes, io_callback,
                                       true);
  } else {
    rv = partial->CacheWrite(entry_->disk_entry, read_buf_.get(), num_bytes,
                             io_callback);
  }
  return rv;
}

int HttpCache::Writers::DoCacheWriteDataComplete(int result) {
  next_state_ = State::NONE;
  if (result != write_len_) {
    OnCacheWriteFailure();

    // |active_transaction_| can continue reading from the network.
    result = write_len_;
  } else {
    OnDataReceived(result);
  }
  return result;
}

int HttpCache::Writers::DoCacheWriteTruncatedResponse() {
  next_state_ = State::CACHE_WRITE_TRUNCATED_RESPONSE_COMPLETE;
  scoped_refptr<PickledIOBuffer> data(new PickledIOBuffer());
  response_info_truncation_->Persist(data->pickle(),
                                     true /* skip_transient_headers*/, true);
  data->Done();
  io_buf_len_ = data->pickle()->size();
  CompletionCallback io_callback =
      base::Bind(&HttpCache::Writers::OnIOComplete, weak_factory_.GetWeakPtr());
  return entry_->disk_entry->WriteData(kResponseInfoIndex, 0, data.get(),
                                       io_buf_len_, io_callback, true);
}

int HttpCache::Writers::DoCacheWriteTruncatedResponseComplete(int result) {
  next_state_ = State::NONE;
  if (result != io_buf_len_) {
    DLOG(ERROR) << "failed to write response info to cache";

    cache_callback_ =
        base::BindOnce(&HttpCache::DoneWithEntryWriters, cache_->GetWeakPtr(),
                       entry_, false, active_transaction_);
    keep_entry_ = false;
  }
  return OK;
}

void HttpCache::Writers::OnDataReceived(int result) {
  // If active_transaction_ has been destroyed and there is no other
  // transaction, mark the entry as truncated.
  if (all_writers_.empty() && result > 0) {
    TruncateEntryInternal();
    cache_callback_ =
        base::BindOnce(&HttpCache::DoneWithEntryWriters, cache_->GetWeakPtr(),
                       entry_, false, active_transaction_);
  }

  bool is_partial = active_transaction_ != nullptr &&
                    active_transaction_->partial() != nullptr;

  // Partial transaction will process the result, return from here.
  if (is_partial) {
    active_transaction_ = nullptr;
    return;
  }

  if (result == 0) {
    // Check if the response is actually completed or if not, attempt to mark
    // the entry as truncated in OnNetworkReadFailure.
    int current_size = entry_->disk_entry->GetDataSize(kResponseContentIndex);
    DCHECK(network_transaction_);
    const HttpResponseInfo* response_info =
        network_transaction_->GetResponseInfo();
    int64_t content_length = response_info->headers->GetContentLength();
    if (content_length >= 0 && content_length > current_size) {
      OnNetworkReadFailure(result);
      return;
    }

    // Collect metrics before entry is possibly destroyed in cache_callback_.
    if (active_transaction_)
      active_transaction_->RecordHistograms();

    // Invoke entry processing. Note it will also remove active_transaction_
    // from |this|.
    cache_callback_ =
        base::BindOnce(&HttpCache::DoneWithEntryWriters, cache_->GetWeakPtr(),
                       entry_, true, active_transaction_);
  }

  // Notify waiting_for_read_. Tasks will be posted for all the
  // transactions.
  ProcessWaitingForReadTransactions(write_len_);

  active_transaction_ = nullptr;
}

void HttpCache::Writers::OnCacheWriteFailure() {
  DLOG(ERROR) << "failed to write response data to cache";

  ProcessFailure(active_transaction_, ERR_CACHE_WRITE_FAILURE);

  // Now writers will only be reading from the network.
  network_read_only_ = true;

  active_transaction_ = nullptr;

  // Call the cache_ function here even if |active_transaction_| is alive
  // because it wouldn't know if this was an error case, since it gets a
  // positive result back. We do not want to remove the active_transaction from
  // writers since it needs to continue reading from the network, so sending
  // nullptr to DoneWritingToEntry so that the entry is only doomed.
  cache_callback_ =
      base::BindOnce(&HttpCache::DoneWithEntryWriters, cache_->GetWeakPtr(),
                     entry_, false, active_transaction_);
  keep_entry_ = false;
}

void HttpCache::Writers::ProcessWaitingForReadTransactions(int result) {
  for (auto& waiting : waiting_for_read_) {
    Transaction* transaction = waiting.transaction;
    int callback_result = result;

    if (result >= 0) {  // success
      // Save the data in the waiting transaction's read buffer.
      waiting.write_len = std::min(waiting.read_buf_len, result);
      memcpy(waiting.read_buf->data(), read_buf_->data(), waiting.write_len);
      callback_result = waiting.write_len;
    }

    // If its response completion or failure, this transaction needs to be
    // removed.
    if (result <= 0)
      all_writers_.erase(transaction);

    // Post task to notify transaction.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(waiting.callback, callback_result));
  }

  waiting_for_read_.clear();
}

void HttpCache::Writers::SetIdleWritersFailState(int result) {
  // Since this is only for idle transactions, waiting_for_read_
  // should be empty.
  DCHECK(waiting_for_read_.empty());
  for (auto* transaction : all_writers_) {
    if (transaction == active_transaction_)
      continue;
    transaction->SetSharedWritingFailState(result);
    all_writers_.erase(transaction);
  }
}

void HttpCache::Writers::ResetStateForEmptyWriters() {
  DCHECK(all_writers_.empty());
  network_read_only_ = false;
  network_transaction_.reset();
}

void HttpCache::Writers::OnIOComplete(int result) {
  DoLoop(result);
}

}  // namespace net
