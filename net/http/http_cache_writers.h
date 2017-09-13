// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_CACHE_WRITERS_H_
#define NET_HTTP_HTTP_CACHE_WRITERS_H_

#include <list>
#include <memory>

#include "base/memory/weak_ptr.h"
#include "net/base/completion_callback.h"
#include "net/http/http_cache.h"
#include "net/http/http_cache_transaction_writers_common.h"

namespace net {

class HttpResponseInfo;

using http_cache_transaction_util::WritersTransactionContext;
using http_cache_transaction_util::NetworkTransactionInfo;

// If multiple HttpCache::Transactions are accessing the same cache entry
// simultaneously, their access to the data read from network is synchronized
// by HttpCache::Writers. This enables each of those transactions to drive
// reading the response body from the network ensuring a slow consumer does not
// starve other consumers of the same resource.
//
// Writers represents the set of all HttpCache::Transactions that are
// reading from the network using the same network transaction and writing to
// the same cache entry. It is owned by the ActiveEntry.
class NET_EXPORT_PRIVATE HttpCache::Writers {
 public:
  // |cache| and |entry| must outlive this object.
  Writers(HttpCache* cache, HttpCache::ActiveEntry* entry);
  ~Writers();

  // Retrieves data from the network transaction associated with the Writers
  // object. This may be done directly (via a network read into |*buf->data()|)
  // or indirectly (by copying from another transactions buffer into
  // |*buf->data()| on network read completion) depending on whether or not a
  // read is currently in progress. May return the result synchronously or
  // return ERR_IO_PENDING: if ERR_IO_PENDING is returned, |callback| will be
  // run to inform the consumer of the result of the Read().
  // |transaction| may be removed while Read() is ongoing. In that case Writers
  // will still complete the Read() processing but will not invoke the
  // |callback|.
  int Read(scoped_refptr<IOBuffer> buf,
           int buf_len,
           const CompletionCallback& callback,
           Transaction* transaction);

  // Invoked when StopCaching is called on a member transaction.
  // It stops caching only if there are no other transactions. Returns true if
  // caching can be stopped.
  bool StopCaching(Transaction* transaction);

  // Invoked when DoneReading is called on a member transaction. It is a no-op
  // if there is an active transaction. Can only be invoked when |transaction|
  // is not the current active transaction.
  void DoneReading(Transaction* transaction);

  // Adds an HttpCache::Transaction to Writers.
  // Should only be invoked if CanAddWriters() returns true.
  // If |is_exclusive| is true, it makes writing an exclusive operation
  // implying that Writers can contain at most one transaction till the
  // completion of the response body.
  // If |network_read_only| is true, then writers will only read from the
  // network and not write to the cache. Obviously such a writers can only have
  // one writer transaction so it must be exclusive.
  // |transaction| can be destroyed at any point and it should invoke
  // RemoveTransaction() during its destruction. This will also ensure any
  // pointers in |info| are not accessed after the transaction is destroyed.
  void AddTransaction(Transaction* transaction,
                      bool is_exclusive,
                      bool network_read_only,
                      RequestPriority priority,
                      std::unique_ptr<WritersTransactionContext> info);

  // Removes a transaction. Should be invoked when this transaction is
  // destroyed. |response_info| could be used for marking the entry as
  // truncated.
  void RemoveTransaction(Transaction* transaction);

  // Invoked when there is a change in a member transaction's priority or a
  // member transaction is removed.
  void UpdatePriority();

  // Returns true if this object is empty.
  bool IsEmpty() const { return all_writers_.empty(); }

  // Invoked during HttpCache's destruction.
  void Clear() { all_writers_.clear(); }

  // Returns true if |transaction| is part of writers.
  bool HasTransaction(const Transaction* transaction) const {
    return all_writers_.count(const_cast<Transaction*>(transaction)) > 0;
  }

  // Remove and return any idle writers. Should only be invoked when a
  // response is completely written and when ContainesOnlyIdleWriters()
  // returns true.
  TransactionSet RemoveAllIdleWriters();

  // Returns true if more writers can be added for shared writing.
  bool CanAddWriters();

  // Returns if only one transaction can be a member of writers.
  bool IsExclusive() { return is_exclusive_; }

  // Returns the network transaction which may be null for range requests.
  const HttpTransaction* network_transaction() const {
    return network_transaction_.get();
  }

  // Invoked to mark an entry as truncated. Returns true and is a no-op if there
  // is an ongoing async operation. Returns true if either the entry is marked
  // as truncated or the entry is already completely written and false if the
  // entry cannot be resumed later so no need to mark truncated. If writers is
  // in the midst of an async operation, truncation would be handled at the end
  // of the operation or if there are more transactions, then there is no need
  // to truncate, and thus this will be a no-op.
  void TruncateEntry();

  // Should be invoked only when writers has transactions attached to it and
  // thus has a valid network transaction.
  LoadState GetWriterLoadState();

  // Sets the network transaction argument to |network_transaction_|. Must be
  // invoked before Read can be invoked.
  void SetNetworkTransaction(
      Transaction* transaction,
      std::unique_ptr<HttpTransaction> network_transaction);

  // Resets the network transaction to null. Required for range requests as they
  // might use the current network transaction only for part of the request.
  // Must only be invoked for partial requests.
  void ResetPartialNetworkTransaction();

  // Sets network_read_only_ which implies the response will not be subsequently
  // written to the cache. If |success| is false, the entry will be doomed else
  // attempt to truncate the entry will be done. May only be called if the
  // calling transaction is present in writers.  If writers is shared, this is
  // a no-op.
  void MaybeSetNetworkReadOnly(bool success);

  // Returns if response is only being read from the network.
  bool network_read_only() const { return network_read_only_; }

  // Returns true if entry is worth keeping in its current state. It may be
  // truncated or complete. Returns false if entry is incomplete and cannot be
  // resumed, it will not be marked as truncated and will not be preserved.
  bool ShouldKeepEntry() const { return should_keep_entry_; }

  int GetTransactionsCount() const { return all_writers_.size(); }

  // For unit testing.
  enum class TruncateResultForTesting {
    NONE,
    NO_OP_MORE_TRANSACTIONS,
    FAIL_CANNOT_RESUME,
  };

  TruncateResultForTesting truncate_result_for_testing() {
    return truncate_result_for_testing_;
  }

 private:
  friend class WritersTest;

  enum class State {
    UNSET,
    NONE,
    NETWORK_READ,
    NETWORK_READ_COMPLETE,
    CACHE_WRITE_DATA,
    CACHE_WRITE_DATA_COMPLETE,
    CACHE_WRITE_TRUNCATED_RESPONSE,
    CACHE_WRITE_TRUNCATED_RESPONSE_COMPLETE,
  };

  // These transactions are waiting on Read. After the active transaction
  // completes writing the data to the cache, their buffer would be filled with
  // the data and their callback will be invoked.
  struct WaitingForRead {
    Transaction* transaction;
    scoped_refptr<IOBuffer> read_buf;
    int read_buf_len;
    int write_len;
    const CompletionCallback callback;
    WaitingForRead(Transaction* transaction,
                   scoped_refptr<IOBuffer> read_buf,
                   int len,
                   const CompletionCallback& consumer_callback);
    ~WaitingForRead();
    WaitingForRead(const WaitingForRead&);
  };
  using WaitingForReadList = std::list<WaitingForRead>;

  using TransactionMap =
      std::map<Transaction*, std::unique_ptr<WritersTransactionContext>>;

  // Runs the state transition loop. Resets and calls |callback_| on exit,
  // unless the return value is ERR_IO_PENDING.
  int DoLoop(int result);

  // State machine functions.
  int DoNetworkRead();
  int DoNetworkReadComplete(int result);
  int DoCacheWriteData(int num_bytes);
  int DoCacheWriteDataComplete(int result);
  int DoCacheWriteTruncatedResponse();
  int DoCacheWriteTruncatedResponseComplete(int result);

  // Helper functions for callback.
  void OnNetworkReadFailure(int result);
  void OnCacheWriteFailure();
  void OnDataReceived(int result);

  // Notifies the transactions waiting on Read of the result, by posting a task
  // for each of them.
  void ProcessWaitingForReadTransactions(int result);

  // Sets the state to FAIL_READ so that any subsequent Read on an idle
  // transaction fails.
  void SetIdleWritersFailState(int result);

  // Called to reset state when all transaction references are removed from
  // |this|.
  void ResetStateForEmptyWriters();

  // Invoked when |active_transaction_| fails to read from network or write to
  // cache. |error| indicates network read error code or cache write error.
  void ProcessFailure(int error);

  // Returns true if |this| only contains idle writers. Idle writers are those
  // that are waiting for Read to be invoked by the consumer.
  bool ContainsOnlyIdleWriters() const;

  // Returns true if its worth marking the entry as truncated.
  bool CanResume() const;

  // Invoked for truncating the entry either internally within DoLoop or through
  // the API TruncateEntry.
  void TruncateEntryInternal();

  // Remove the transaction.
  void RemoveTransactionInternal(Transaction* transaction);

  TransactionSet RemoveTransactionsFromAllWriters();

  // IO Completion callback function.
  void OnIOComplete(int result);

  State next_state_ = State::NONE;

  // True if only reading from network and not writing to cache.
  bool network_read_only_ = false;

  HttpCache* cache_ = nullptr;

  // Owner of |this|.
  ActiveEntry* entry_ = nullptr;

  std::unique_ptr<HttpTransaction> network_transaction_ = nullptr;

  scoped_refptr<IOBuffer> read_buf_ = nullptr;

  int io_buf_len_ = 0;
  int write_len_ = 0;

  // The cache transaction that is the current consumer of network_transaction_
  // ::Read or writing to the entry and is waiting for the operation to be
  // completed. This is used to ensure there is at most one consumer of
  // network_transaction_ at a time.
  Transaction* active_transaction_ = nullptr;

  // Transactions whose consumers have invoked Read, but another transaction is
  // currently the |active_transaction_|. After the network read and cache write
  // is complete, the waiting transactions will be notified.
  WaitingForReadList waiting_for_read_;

  // Includes all transactions. ResetStateForEmptyWriters should be invoked
  // whenever all_writers_ becomes empty.
  TransactionMap all_writers_;

  // True if multiple transactions are not allowed e.g. for partial requests.
  bool is_exclusive_ = false;

  // Current priority of the request. It is always the maximum of all the writer
  // transactions.
  RequestPriority priority_ = MINIMUM_PRIORITY;

  // Response info of the most recent transaction added to Writers will be used
  // to write back the headers along with the truncated bit set. This is done so
  // that we don't overwrite headers written by a more recent transaction with
  // older headers while truncating.
  HttpResponseInfo response_info_truncation_;

  // Do not mark a partial request as truncated if it is not already a truncated
  // entry to start with.
  bool partial_do_not_truncate_ = false;

  // True if the entry should be kept, even if the response was not completely
  // written.
  bool should_keep_entry_ = true;

  // True if the Transaction is currently processing the DoLoop.
  bool is_in_do_loop_ = false;

  // Truncate result for testing.
  TruncateResultForTesting truncate_result_for_testing_ =
      TruncateResultForTesting::NONE;

  CompletionCallback callback_;  // Callback for active_transaction_.

  // Since cache_ can destroy |this|, |cache_callback_| is only invoked at the
  // end of DoLoop().
  base::OnceClosure cache_callback_;  // Callback for cache_.

  base::WeakPtrFactory<Writers> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(Writers);
};

}  // namespace net

#endif  // NET_HTTP_HTTP_CACHE_WRITERS_H_
