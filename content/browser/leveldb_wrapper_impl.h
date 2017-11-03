// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LEVELDB_WRAPPER_IMPL_H_
#define CONTENT_BROWSER_LEVELDB_WRAPPER_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace base {
namespace trace_event {
class ProcessMemoryDump;
}
}

namespace content {

// Pure vitual class given to the delagate to perform operations on the
// wrapper implementation. Since operations & the delegate can outlive the
// wrapper, the object passed to the delegate is an internal class instead.
class LevelDBWrapperControl {
 public:
  ~LevelDBWrapperControl();

  virtual bool empty() const = 0;
  virtual size_t bytes_used() const = 0;

  virtual void ScheduleImmediateCommit() = 0;
};

// This is a wrapper around a leveldb::mojom::LevelDBDatabase. Multiple
// interface
// pointers can be bound to the same object. The wrapper adds a couple of
// features not found directly in leveldb.
// 1) Adds the given prefix, if any, to all keys. This allows the sharing of one
//    database across many, possibly untrusted, consumers and ensuring that they
//    can't access each other's values.
// 2) Enforces a max_size constraint.
// 3) Informs observers when values scoped by prefix are modified.
// 4) Throttles requests to avoid overwhelming the disk.
class CONTENT_EXPORT LevelDBWrapperImpl : public mojom::LevelDBWrapper {
 public:
  using ValueMap = std::map<std::vector<uint8_t>, std::vector<uint8_t>>;
  using ValueMapCallback = base::OnceCallback<void(std::unique_ptr<ValueMap>)>;
  using Change =
      std::pair<std::vector<uint8_t>, base::Optional<std::vector<uint8_t>>>;

  // Note: This class can be called after the destruction of the
  // LevelDBWrapperImpl, so the delegate should never access the
  // LevelDBWrapperImpl.
  class CONTENT_EXPORT Delegate {
   public:
    virtual ~Delegate();
    // This given control is valid for the lifetime of this object.
    virtual void Initialize(LevelDBWrapperControl* control) = 0;
    virtual void OnNoBindings() = 0;
    virtual std::vector<leveldb::mojom::BatchedOperationPtr>
    PrepareToCommit() = 0;
    virtual void DidCommit(leveldb::mojom::DatabaseError error) = 0;
    // Called during loading if no data was found. Needs to call |callback|.
    virtual void MigrateData(ValueMapCallback callback);
    // Called during loading to give delegate a chance to modify the data as
    // stored in the database.
    virtual std::vector<Change> FixUpData(const ValueMap& data);
    virtual void OnMapLoaded(leveldb::mojom::DatabaseError error);
  };

  // |no_bindings_callback| will be called when this object has no more
  // bindings and all pending modifications have been processed.
  LevelDBWrapperImpl(leveldb::mojom::LevelDBDatabase* database,
                     const std::string& prefix,
                     size_t max_size,
                     base::TimeDelta default_commit_delay,
                     int max_bytes_per_hour,
                     int max_commits_per_hour,
                     std::unique_ptr<Delegate> delegate);
  ~LevelDBWrapperImpl() override;

  void Bind(mojom::LevelDBWrapperRequest request);

  bool empty() const;
  size_t bytes_used() const;

  bool has_pending_load_tasks() const;

  // Commence aggressive flushing. This should be called early during startup,
  // before any localStorage writing. Currently scheduled writes will not be
  // rescheduled and will be flushed at the scheduled time after which
  // aggressive flushing will commence.
  static void EnableAggressiveCommitDelay();

  // Commits any uncommitted data to the database as soon as possible. This
  // usually means data will be committed immediately, but if we're currently
  // waiting on the result of initializing our map the commit won't happen
  // until the load has finished.
  void ScheduleImmediateCommit();

  // Clears the in-memory cache if currently no changes are pending. If there
  // are uncommitted changes this method does nothing.
  void PurgeMemory();

  // Adds memory statistics to |pmd| for memory infra.
  void OnMemoryDump(const std::string& name,
                    base::trace_event::ProcessMemoryDump* pmd);

  // LevelDBWrapper:
  void AddObserver(mojom::LevelDBObserverAssociatedPtrInfo observer) override;
  void Put(const std::vector<uint8_t>& key,
           const std::vector<uint8_t>& value,
           const base::Optional<std::vector<uint8_t>>& client_old_value,
           const std::string& source,
           PutCallback callback) override;
  void Delete(const std::vector<uint8_t>& key,
              const base::Optional<std::vector<uint8_t>>& client_old_value,
              const std::string& source,
              DeleteCallback callback) override;
  void DeleteAll(const std::string& source,
                 DeleteAllCallback callback) override;
  void Get(const std::vector<uint8_t>& key, GetCallback callback) override;
  void GetAll(
      mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
      GetAllCallback callback) override;

 private:
  // All state is maintained in an inner class so that deletion doesn't drop
  // db operations.
  class Worker;

  // Used to rate limit commits.
  class RateLimiter {
   public:
    RateLimiter(size_t desired_rate, base::TimeDelta time_quantum);

    void add_samples(size_t samples) { samples_ += samples;  }

    // Computes the total time needed to process the total samples seen
    // at the desired rate.
    base::TimeDelta ComputeTimeNeeded() const;

    // Given the elapsed time since the start of the rate limiting session,
    // computes the delay needed to mimic having processed the total samples
    // seen at the desired rate.
    base::TimeDelta ComputeDelayNeeded(
        const base::TimeDelta elapsed_time) const;

   private:
    float rate_;
    float samples_;
    base::TimeDelta time_quantum_;
  };

  struct CommitBatch {
    bool clear_all_first;
    std::set<std::vector<uint8_t>> changed_keys;

    CommitBatch();
    ~CommitBatch();
  };

  void OnConnectionError();
  void OnLoadComplete();

  scoped_refptr<Worker> worker_;
  mojo::BindingSet<mojom::LevelDBWrapper> bindings_;
  base::WeakPtrFactory<LevelDBWrapperImpl> weak_ptr_factory_;

  static bool s_aggressive_flushing_enabled_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBWrapperImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LEVELDB_WRAPPER_IMPL_H_
