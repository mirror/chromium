// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/origin_manifest/sqlite_persistent_origin_manifest_store.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "sql/error_delegate_util.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "url/gurl.h"

using base::Time;

namespace {

#if defined(OS_IOS)
const int kLoadDelayMilliseconds = 0;
#else
const int kLoadDelayMilliseconds = 0;
#endif

}  // namespace

namespace origin_manifest {

class SQLitePersistentOriginManifestStore::Backend
    : public base::RefCountedThreadSafe<
          SQLitePersistentOriginManifestStore::Backend> {
 public:
  Backend(
      const base::FilePath& path,
      const scoped_refptr<base::SequencedTaskRunner>& client_task_runner,
      const scoped_refptr<base::SequencedTaskRunner>& background_task_runner)
      : path_(path),
        num_pending_(0),
        initialized_(false),
        corruption_detected_(false),
        num_origin_manifests_read_(0),
        client_task_runner_(client_task_runner),
        background_task_runner_(background_task_runner),
        num_priority_waiting_(0),
        total_priority_requests_(0) {}

  // Creates or loads the SQLite database.
  void Load(const LoadedCallback& loaded_callback);

  // Loads the origin manifest for the origin.
  void LoadOriginManifestForOrigin(const std::string& origin,
                                   const LoadedCallback& loaded_callback);

  // Create an origin manifest from the result of |statement|. This method
  // also updates |num_origin_manifests_read_|.
  mojom::OriginManifestPtr MakeOriginManifestFromSQLStatement(
      sql::Statement* statement);

  // Batch an origin manifest addition.
  void AddOriginManifest(const mojom::OriginManifestPtr& om);

  // Batch an origin manifest deletion.
  void DeleteOriginManifest(const mojom::OriginManifestPtr& om);

  /*
    // Sets callback to run at the beginning of Commit.
    void SetBeforeFlushCallback(base::RepeatingClosure callback);

    // Commit pending operations as soon as possible.
    void Flush(base::OnceClosure callback);
  */

  // Commit any pending operations and close the database.  This must be called
  // before the object is destructed.
  void Close(const base::Closure& callback);

  // Post background delete of all origin manifests which origin matches
  // |origins|.
  void DeleteAllInList(const std::list<std::string>& origins);

 private:
  friend class base::RefCountedThreadSafe<
      SQLitePersistentOriginManifestStore::Backend>;

  // You should call Close() before destructing this object.
  ~Backend() {
    DCHECK(!db_.get()) << "Close should have already been called.";
    DCHECK_EQ(0u, num_pending_);
    DCHECK(pending_.empty());
  }

  // Database upgrade statements.
  bool EnsureDatabaseVersion();

  class PendingOperation {
   public:
    enum OperationType {
      ORIGIN_MANIFEST_ADD,
      ORIGIN_MANIFEST_DELETE,
    };

    PendingOperation(OperationType op, const mojom::OriginManifestPtr& om)
        : op_(op) {
      om_ = om.Clone();
    }

    OperationType op() const { return op_; }
    const mojom::OriginManifestPtr& om() const { return om_; }

   private:
    OperationType op_;
    mojom::OriginManifestPtr om_;
  };

 private:
  // Creates or loads the SQLite database on background runner.
  void LoadAndNotifyInBackground(const LoadedCallback& loaded_callback,
                                 const base::Time& posted_at);

  // Loads origin manifest for the origin on background runner.
  void LoadOriginAndNotifyInBackground(const std::string& origin,
                                       const LoadedCallback& loaded_callback,
                                       const base::Time& posted_at);

  // Notifies the OriginManifestStore when loading completes for a specific
  // origin or for all origins. Triggers the callback and passes it all origin
  // manifests that have been loaded from DB since last IO notification.
  void Notify(const LoadedCallback& loaded_callback, bool load_success);

  /*
    // Flushes (Commits) pending operations on the background runner, and
    invokes
    // |callback| on the client thread when done.
    void FlushAndNotifyInBackground(base::OnceClosure callback);
  */

  // Sends notification when the entire store is loaded, and reports metrics
  // for the total time to load and aggregated results from any priority loads
  // that occurred.
  void CompleteLoadInForeground(const LoadedCallback& loaded_callback,
                                bool load_success);

  // Sends notification when a single priority load completes. Updates priority
  // load metric data. The data is sent only after the final load completes.
  void CompleteLoadForOriginInForeground(const LoadedCallback& loaded_callback,
                                         bool load_success,
                                         const base::Time& requested_at);

  // Sends all metrics, including posting a ReportMetricsInBackground task.
  // Called after all priority and regular loading is complete.
  void ReportMetrics();

  // Sends background-runner owned metrics (i.e., the combined duration of all
  // BG-runner tasks).
  void ReportMetricsInBackground();

  // Initialize the data base.
  bool InitializeDatabase();

  // Loads origin manifests for the next origin from the DB, then either
  // reschedules itself or schedules the provided callback to run on the client
  // runner (if all domains are loaded).
  void ChainLoadOriginManifests(const LoadedCallback& loaded_callback);

  // Load the origin manifest for a set ofdomain
  bool LoadOriginManifestForOriginFromDB(const std::string& origin);

  // Batch a origin manifest operation (add or delete)
  void BatchOperation(PendingOperation::OperationType op,
                      const mojom::OriginManifestPtr& om);
  // Commit our pending operations to the database.
  void Commit();
  // Close() executed on the background runner.
  void InternalBackgroundClose(const base::Closure& callback);

  /*
    void DeleteSessionCookiesOnStartup();
  */

  void BackgroundDeleteAllInList(const std::list<std::string>& origins);

  void DatabaseErrorCallback(int error, sql::Statement* stmt);
  void KillDatabase();

  void PostBackgroundTask(const tracked_objects::Location& origin,
                          base::OnceClosure task);
  void PostClientTask(const tracked_objects::Location& origin,
                      base::OnceClosure task);

  // Shared code between the different load strategies to be used after all
  // origin manifests have been loaded.
  void FinishedLoadingOriginManifests(const LoadedCallback& loaded_callback,
                                      bool success);

  const base::FilePath path_;
  std::unique_ptr<sql::Connection> db_;
  sql::MetaTable meta_table_;

  typedef std::list<PendingOperation*> PendingOperationsList;
  PendingOperationsList pending_;
  PendingOperationsList::size_type num_pending_;
  // Guard |origin_manifests_|, |pending_|, |num_pending_|.
  base::Lock lock_;

  // Temporary buffer for origin manifests loaded from DB. Accumulates origin
  // manifests to reduce the number of messages sent to the client runner.
  // Sent back in response to individual load requests for origin keys or
  // when all loading completes.
  std::vector<mojom::OriginManifestPtr> origin_manifests_;

  // Vector of origins that are to be loaded from DB.
  std::vector<std::string> origins_to_load_;

  // Indicates if DB has been initialized.
  bool initialized_;

  // Indicates if the kill-database callback has been scheduled.
  bool corruption_detected_;

  /*
    // If false, we should filter out session cookies when reading the DB.
    bool restore_old_session_cookies_;
  */

  // The cumulative time spent loading the origin manifests on the background
  // runner. Incremented and reported from the background runner.
  base::TimeDelta origin_manifest_load_duration_;

  // The total number of origin manifests read. Incremented and reported on the
  // background runner.
  int num_origin_manifests_read_;

  scoped_refptr<base::SequencedTaskRunner> client_task_runner_;
  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;

  // Guards the following metrics-related properties (only accessed when
  // starting/completing priority loads or completing the total load).
  base::Lock metrics_lock_;
  int num_priority_waiting_;
  // The total number of priority requests.
  int total_priority_requests_;
  // The time when |num_priority_waiting_| incremented to 1.
  base::Time current_priority_wait_start_;
  // The cumulative duration of time when |num_priority_waiting_| was greater
  // than 1.
  base::TimeDelta priority_wait_duration_;
  /*
    // Class with functions that do cryptographic operations (for protecting
    // cookies stored persistently).
    //
    // Not owned.
    CookieCryptoDelegate* crypto_;
  */
  // Callback to run before Commit.
  base::RepeatingClosure before_flush_callback_;
  // Guards |before_flush_callback_|.
  base::Lock before_flush_callback_lock_;

  DISALLOW_COPY_AND_ASSIGN(Backend);
};

namespace {

// Version number of the database.
const int kCurrentVersionNumber = 1;
const int kCompatibleVersionNumber = 1;

/*
// Possible values for the 'priority' column.
enum DBCookiePriority {
  kCookiePriorityLow = 0,
  kCookiePriorityMedium = 1,
  kCookiePriorityHigh = 2,
};

DBCookiePriority CookiePriorityToDBCookiePriority(CookiePriority value) {
  switch (value) {
    case COOKIE_PRIORITY_LOW:
      return kCookiePriorityLow;
    case COOKIE_PRIORITY_MEDIUM:
      return kCookiePriorityMedium;
    case COOKIE_PRIORITY_HIGH:
      return kCookiePriorityHigh;
  }

  NOTREACHED();
  return kCookiePriorityMedium;
}

CookiePriority DBCookiePriorityToCookiePriority(DBCookiePriority value) {
  switch (value) {
    case kCookiePriorityLow:
      return COOKIE_PRIORITY_LOW;
    case kCookiePriorityMedium:
      return COOKIE_PRIORITY_MEDIUM;
    case kCookiePriorityHigh:
      return COOKIE_PRIORITY_HIGH;
  }

  NOTREACHED();
  return COOKIE_PRIORITY_DEFAULT;
}

// Possible values for the 'samesite' column
enum DBCookieSameSite {
  kCookieSameSiteNoRestriction = 0,
  kCookieSameSiteLax = 1,
  kCookieSameSiteStrict = 2,
};

DBCookieSameSite CookieSameSiteToDBCookieSameSite(CookieSameSite value) {
  switch (value) {
    case CookieSameSite::NO_RESTRICTION:
      return kCookieSameSiteNoRestriction;
    case CookieSameSite::LAX_MODE:
      return kCookieSameSiteLax;
    case CookieSameSite::STRICT_MODE:
      return kCookieSameSiteStrict;
  }

  NOTREACHED();
  return kCookieSameSiteNoRestriction;
}

CookieSameSite DBCookieSameSiteToCookieSameSite(DBCookieSameSite value) {
  switch (value) {
    case kCookieSameSiteNoRestriction:
      return CookieSameSite::NO_RESTRICTION;
    case kCookieSameSiteLax:
      return CookieSameSite::LAX_MODE;
    case kCookieSameSiteStrict:
      return CookieSameSite::STRICT_MODE;
  }

  NOTREACHED();
  return CookieSameSite::DEFAULT_MODE;
}
*/

// Increments a specified TimeDelta by the duration between this object's
// constructor and destructor. Not thread safe. Multiple instances may be
// created with the same delta instance as long as their lifetimes are nested.
// The shortest lived instances have no impact.
class IncrementTimeDelta {
 public:
  explicit IncrementTimeDelta(base::TimeDelta* delta)
      : delta_(delta), original_value_(*delta), start_(base::Time::Now()) {}

  ~IncrementTimeDelta() {
    *delta_ = original_value_ + base::Time::Now() - start_;
  }

 private:
  base::TimeDelta* delta_;
  base::TimeDelta original_value_;
  base::Time start_;

  DISALLOW_COPY_AND_ASSIGN(IncrementTimeDelta);
};

// Initializes the originmanifests table, returning true on success.
bool InitTable(sql::Connection* db) {
  if (db->DoesTableExist("originmanifests"))
    return true;

  std::string stmt(
      base::StringPrintf("CREATE TABLE originmanifests ("
                         "origin TEXT NOT NULL UNIQUE PRIMARY KEY,"
                         "version TEXT NOT NULL,"
                         "value TEXT NOT NULL)"));
  //"value TEXT NOT NULL,"
  //"creation_utc INTEGER NOT NULL,"
  //"expires_utc INTEGER NOT NULL)"));
  if (!db->Execute(stmt.c_str()))
    return false;

  return true;
}

}  // namespace

void SQLitePersistentOriginManifestStore::Backend::Load(
    const LoadedCallback& loaded_callback) {
  PostBackgroundTask(FROM_HERE,
                     base::Bind(&Backend::LoadAndNotifyInBackground, this,
                                loaded_callback, base::Time::Now()));
}

void SQLitePersistentOriginManifestStore::Backend::LoadOriginManifestForOrigin(
    const std::string& origin,
    const LoadedCallback& loaded_callback) {
  {
    base::AutoLock locked(metrics_lock_);
    if (num_priority_waiting_ == 0)
      current_priority_wait_start_ = base::Time::Now();
    num_priority_waiting_++;
    total_priority_requests_++;
  }

  PostBackgroundTask(FROM_HERE,
                     base::Bind(&Backend::LoadOriginAndNotifyInBackground, this,
                                origin, loaded_callback, base::Time::Now()));
}

void SQLitePersistentOriginManifestStore::Backend::LoadAndNotifyInBackground(
    const LoadedCallback& loaded_callback,
    const base::Time& posted_at) {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());
  IncrementTimeDelta increment(&origin_manifest_load_duration_);

  /*
  UMA_HISTOGRAM_CUSTOM_TIMES("OriginManifest.TimeLoadDBQueueWait",
                             base::Time::Now() - posted_at,
                             base::TimeDelta::FromMilliseconds(1),
                             base::TimeDelta::FromMinutes(1), 50);
  */

  if (!InitializeDatabase()) {
    PostClientTask(FROM_HERE, base::Bind(&Backend::CompleteLoadInForeground,
                                         this, loaded_callback, false));
  } else {
    ChainLoadOriginManifests(loaded_callback);
  }
}

void SQLitePersistentOriginManifestStore::Backend::
    LoadOriginAndNotifyInBackground(const std::string& origin,
                                    const LoadedCallback& loaded_callback,
                                    const base::Time& posted_at) {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());
  IncrementTimeDelta increment(&origin_manifest_load_duration_);

  /*
  UMA_HISTOGRAM_CUSTOM_TIMES("OriginManifest.TimeKeyLoadDBQueueWait",
                             base::Time::Now() - posted_at,
                             base::TimeDelta::FromMilliseconds(1),
                             base::TimeDelta::FromMinutes(1), 50);
  */

  bool success = false;
  if (InitializeDatabase()) {
    std::vector<std::string>::iterator it =
        std::find(origins_to_load_.begin(), origins_to_load_.end(), origin);
    if (it != origins_to_load_.end()) {
      success = LoadOriginManifestForOriginFromDB(origin);
      origins_to_load_.erase(it);
    } else {
      success = true;
    }
  }

  PostClientTask(FROM_HERE,
                 base::Bind(&SQLitePersistentOriginManifestStore::Backend::
                                CompleteLoadForOriginInForeground,
                            this, loaded_callback, success, posted_at));
}

/*
void SQLitePersistentOriginManifestStore::Backend::FlushAndNotifyInBackground(
    base::OnceClosure callback) {
  Commit();
  if (!callback.is_null())
    PostClientTask(FROM_HERE, std::move(callback));
}
*/

void SQLitePersistentOriginManifestStore::Backend::
    CompleteLoadForOriginInForeground(const LoadedCallback& loaded_callback,
                                      bool load_success,
                                      const ::Time& requested_at) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());

  /*
  UMA_HISTOGRAM_CUSTOM_TIMES("OriginManifest.TimeKeyLoadTotalWait",
                             base::Time::Now() - requested_at,
                             base::TimeDelta::FromMilliseconds(1),
                             base::TimeDelta::FromMinutes(1), 50);
  */

  Notify(loaded_callback, load_success);

  {
    base::AutoLock locked(metrics_lock_);
    num_priority_waiting_--;
    if (num_priority_waiting_ == 0) {
      priority_wait_duration_ +=
          base::Time::Now() - current_priority_wait_start_;
    }
  }
}

void SQLitePersistentOriginManifestStore::Backend::ReportMetricsInBackground() {
  /*
  UMA_HISTOGRAM_CUSTOM_TIMES("OriginManifest.TimeLoad",
                             origin_manifest_load_duration_,
                             base::TimeDelta::FromMilliseconds(1),
                             base::TimeDelta::FromMinutes(1), 50);
  */
}

void SQLitePersistentOriginManifestStore::Backend::ReportMetrics() {
  PostBackgroundTask(FROM_HERE,
                     base::Bind(&SQLitePersistentOriginManifestStore::Backend::
                                    ReportMetricsInBackground,
                                this));

  /*
  {
    base::AutoLock locked(metrics_lock_);
    UMA_HISTOGRAM_CUSTOM_TIMES("OriginManifest.PriorityBlockingTime",
                               priority_wait_duration_,
                               base::TimeDelta::FromMilliseconds(1),
                               base::TimeDelta::FromMinutes(1), 50);

    UMA_HISTOGRAM_COUNTS_100("OriginManifest.PriorityLoadCount",
                             total_priority_requests_);

    UMA_HISTOGRAM_COUNTS_10000("OriginManifest.NumberOfLoadedOriginManifests",
                               num_origin_manifests_read_);
  }
  */
}

void SQLitePersistentOriginManifestStore::Backend::CompleteLoadInForeground(
    const LoadedCallback& loaded_callback,
    bool load_success) {
  Notify(loaded_callback, load_success);

  if (load_success)
    ReportMetrics();
}

void SQLitePersistentOriginManifestStore::Backend::Notify(
    const LoadedCallback& loaded_callback,
    bool load_success) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());

  std::vector<mojom::OriginManifestPtr> origin_manifests;
  {
    base::AutoLock locked(lock_);
    origin_manifests.swap(origin_manifests_);
  }

  loaded_callback.Run(std::move(origin_manifests));
}

bool SQLitePersistentOriginManifestStore::Backend::InitializeDatabase() {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());

  if (initialized_ || corruption_detected_) {
    // Return false if we were previously initialized but the DB has since been
    // closed, or if corruption caused a database reset during initialization.
    return db_ != NULL;
  }

  base::Time start = base::Time::Now();

  const base::FilePath dir = path_.DirName();
  if (!base::PathExists(dir) && !base::CreateDirectory(dir)) {
    return false;
  }

  /*
  int64_t db_size = 0;
  if (base::GetFileSize(path_, &db_size))
    UMA_HISTOGRAM_COUNTS_1M("OriginManifest.DBSizeInKB", db_size / 1024);
  */

  db_.reset(new sql::Connection);
  db_->set_histogram_tag("OriginManifest");

  // Unretained to avoid a ref loop with |db_|.
  db_->set_error_callback(base::Bind(
      &SQLitePersistentOriginManifestStore::Backend::DatabaseErrorCallback,
      base::Unretained(this)));

  if (!db_->Open(path_)) {
    NOTREACHED() << "Unable to open origin manifest DB.";
    if (corruption_detected_)
      db_->Raze();
    meta_table_.Reset();
    db_.reset();
    return false;
  }

  if (!EnsureDatabaseVersion() || !InitTable(db_.get())) {
    NOTREACHED() << "Unable to open origin manifest DB.";
    if (corruption_detected_)
      db_->Raze();
    meta_table_.Reset();
    db_.reset();
    return false;
  }

  /*
  UMA_HISTOGRAM_CUSTOM_TIMES("OriginManifest.TimeInitializeDB",
                             base::Time::Now() - start,
                             base::TimeDelta::FromMilliseconds(1),
                             base::TimeDelta::FromMinutes(1), 50);
  */

  start = base::Time::Now();

  // Retrieve all the origins
  sql::Statement smt(
      db_->GetUniqueStatement("SELECT DISTINCT origin FROM originmanifests"));

  if (!smt.is_valid()) {
    if (corruption_detected_)
      db_->Raze();
    meta_table_.Reset();
    db_.reset();
    return false;
  }

  while (smt.Step())
    origins_to_load_.push_back(smt.ColumnString(0));

  /*
  UMA_HISTOGRAM_CUSTOM_TIMES("OriginManifest.TimeLoadOrigins",
                             base::Time::Now() - start,
                             base::TimeDelta::FromMilliseconds(1),
                             base::TimeDelta::FromMinutes(1), 50);
  */
  initialized_ = true;

  return true;
}

void SQLitePersistentOriginManifestStore::Backend::ChainLoadOriginManifests(
    const LoadedCallback& loaded_callback) {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());
  IncrementTimeDelta increment(&origin_manifest_load_duration_);

  bool load_success = true;

  if (!db_) {
    // Close() has been called on this store.
    load_success = false;
  } else if (origins_to_load_.size() > 0) {
    // Load origin manifest for the first origin.
    std::vector<std::string>::iterator it = origins_to_load_.begin();
    load_success = LoadOriginManifestForOriginFromDB(*it);
    origins_to_load_.erase(it);
  }

  // If load is successful and there are more origins to be loaded,
  // then post a background task to continue chain-load;
  // Otherwise notify on client runner.
  if (load_success && origins_to_load_.size() > 0) {
    bool success = background_task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&Backend::ChainLoadOriginManifests, this, loaded_callback),
        base::TimeDelta::FromMilliseconds(kLoadDelayMilliseconds));
    if (!success) {
      LOG(WARNING) << "Failed to post task from " << FROM_HERE.ToString()
                   << " to background_task_runner_.";
    }
  } else {
    FinishedLoadingOriginManifests(loaded_callback, load_success);
  }
}

bool SQLitePersistentOriginManifestStore::Backend::
    LoadOriginManifestForOriginFromDB(const std::string& origin) {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());

  sql::Statement smt;
  smt.Assign(db_->GetCachedStatement(
      SQL_FROM_HERE,
      //"SELECT origin, version, value, creation_utc, expires_utc "
      "SELECT origin, version, value "
      "FROM originmanifests WHERE origin = ? "));
  if (!smt.is_valid()) {
    smt.Clear();  // Disconnect smt_ref from db_.
    meta_table_.Reset();
    db_.reset();
    return false;
  }

  smt.BindString(0, origin);
  mojom::OriginManifestPtr origin_manifest =
      MakeOriginManifestFromSQLStatement(&smt);
  if (origin_manifest.Equals(nullptr)) {
    base::AutoLock locked(lock_);
    origin_manifests_.push_back(std::move(origin_manifest));
  }
  return true;
}

mojom::OriginManifestPtr SQLitePersistentOriginManifestStore::Backend::
    MakeOriginManifestFromSQLStatement(sql::Statement* statement) {
  sql::Statement& smt = *statement;
  if (smt.Step()) {
    mojom::OriginManifestPtr om = mojom::OriginManifest::New(
        smt.ColumnString(0),  // origin
        smt.ColumnString(1),  // version
        smt.ColumnString(2),  // json
        mojom::CORSPreflight::New(),
        std::vector<mojom::ContentSecurityPolicyPtr>());
    // om->creationdate = Time::FromInternalValue(smt.ColumnInt64(3));
    // om->expirydate = Time::FromInternalValue(smt.ColumnInt64(4));

    /* Does not support creation and expiration date yet
        DLOG_IF(WARNING, om->creationdate > Time::Now())
            << L"CreationDate too recent";
    */
    ++num_origin_manifests_read_;
    return om;
  }
  return nullptr;
}

bool SQLitePersistentOriginManifestStore::Backend::EnsureDatabaseVersion() {
  // Version check.
  if (!meta_table_.Init(db_.get(), kCurrentVersionNumber,
                        kCompatibleVersionNumber)) {
    return false;
  }

  if (meta_table_.GetCompatibleVersionNumber() > kCurrentVersionNumber) {
    LOG(WARNING) << "Origin Manifest database is too new.";
    return false;
  }

  int cur_version = meta_table_.GetVersionNumber();

  // Put future migration cases here.

  if (cur_version < kCurrentVersionNumber) {
    // UMA_HISTOGRAM_COUNTS_100("OriginManifest.CorruptMetaTable", 1);

    meta_table_.Reset();
    db_.reset(new sql::Connection);
    if (!sql::Connection::Delete(path_) || !db_->Open(path_) ||
        !meta_table_.Init(db_.get(), kCurrentVersionNumber,
                          kCompatibleVersionNumber)) {
      /*
      UMA_HISTOGRAM_COUNTS_100("OriginManifest.CorruptMetaTableRecoveryFailed",
      1);
      */
      NOTREACHED() << "Unable to reset the origin manifest DB.";
      meta_table_.Reset();
      db_.reset();
      return false;
    }
  }

  return true;
}

void SQLitePersistentOriginManifestStore::Backend::AddOriginManifest(
    const mojom::OriginManifestPtr& om) {
  BatchOperation(PendingOperation::ORIGIN_MANIFEST_ADD, om);
}

void SQLitePersistentOriginManifestStore::Backend::DeleteOriginManifest(
    const mojom::OriginManifestPtr& om) {
  BatchOperation(PendingOperation::ORIGIN_MANIFEST_DELETE, om);
}

void SQLitePersistentOriginManifestStore::Backend::BatchOperation(
    PendingOperation::OperationType op,
    const mojom::OriginManifestPtr& om) {
  // Commit every 30 seconds.
  static const int kCommitIntervalMs = 5 * 1000;  // TODO set back to 30
  // Commit right away if we have more than 512 outstanding operations.
  static const size_t kCommitAfterBatchSize = 1;  // TODO set back to 512
  DCHECK(!background_task_runner_->RunsTasksInCurrentSequence());

  // We do a full copy of the origin manifest here, and hopefully just here.
  std::unique_ptr<PendingOperation> po(new PendingOperation(op, om));

  PendingOperationsList::size_type num_pending;
  {
    base::AutoLock locked(lock_);
    pending_.push_back(po.release());
    num_pending = ++num_pending_;
  }

  if (num_pending == 1) {
    // We've gotten our first entry for this batch, fire off the timer.
    if (!background_task_runner_->PostDelayedTask(
            FROM_HERE, base::Bind(&Backend::Commit, this),
            base::TimeDelta::FromMilliseconds(kCommitIntervalMs))) {
      NOTREACHED() << "background_task_runner_ is not running.";
    }
  } else if (num_pending == kCommitAfterBatchSize) {
    // We've reached a big enough batch, fire off a commit now.
    PostBackgroundTask(FROM_HERE, base::Bind(&Backend::Commit, this));
  }
}

void SQLitePersistentOriginManifestStore::Backend::Commit() {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());

  {
    base::AutoLock locked(before_flush_callback_lock_);
    if (!before_flush_callback_.is_null())
      before_flush_callback_.Run();
  }

  PendingOperationsList ops;
  {
    base::AutoLock locked(lock_);
    pending_.swap(ops);
    num_pending_ = 0;
  }

  // Maybe an old timer fired or we are already Close()'ed.
  if (!db_.get() || ops.empty())
    return;

  sql::Statement add_smt(
      db_->GetCachedStatement(SQL_FROM_HERE,
                              "INSERT OR REPLACE INTO originmanifests (origin, "
                              "version, value) "
                              //"version, value, creation_utc, expires_utc) "
                              "VALUES (?,?,?,?,?)"));
  if (!add_smt.is_valid())
    return;

  sql::Statement del_smt(db_->GetCachedStatement(
      SQL_FROM_HERE, "DELETE FROM originmanifests WHERE origin=?"));
  if (!del_smt.is_valid())
    return;

  sql::Transaction transaction(db_.get());
  if (!transaction.Begin())
    return;

  for (PendingOperationsList::iterator it = ops.begin(); it != ops.end();
       ++it) {
    // Free the origin manifests as we commit them to the database.
    std::unique_ptr<PendingOperation> po(*it);
    switch (po->op()) {
      case PendingOperation::ORIGIN_MANIFEST_ADD:
        add_smt.Reset(true);
        add_smt.BindString(0, po->om()->origin);
        add_smt.BindString(1, po->om()->version);
        add_smt.BindString(2, po->om()->json);  // value
        // add_smt.BindInt64(3, po->om()->creationdate);
        // add_smt.BindInt64(4, po->om()->expirydate);
        if (!add_smt.Run())
          NOTREACHED() << "Could not add an origin manifest to the DB.";
        VLOG(1) << "origin manifest added: " << po->om()->origin << ", "
                << po->om()->version << ", " << po->om()->json;
        break;

      case PendingOperation::ORIGIN_MANIFEST_DELETE:
        del_smt.Reset(true);
        del_smt.BindString(0, po->om()->origin);
        if (!del_smt.Run())
          NOTREACHED() << "Could not delete an origin manifest from the DB.";
        break;

      default:
        NOTREACHED();
        break;
    }
  }
  bool succeeded = transaction.Commit();
  VLOG(1) << "Commit succeeded " << succeeded;
  /*
  UMA_HISTOGRAM_ENUMERATION("OriginManifest.BackingStoreUpdateResults",
                            succeeded ? 0 : 1, 2);
  */
}

/*
void SQLitePersistentOriginManifestStore::Backend::SetBeforeFlushCallback(
    base::RepeatingClosure callback) {
  base::AutoLock locked(before_flush_callback_lock_);
  before_flush_callback_ = std::move(callback);
}

void SQLitePersistentOriginManifestStore::Backend::Flush(base::OnceClosure
callback) { DCHECK(!background_task_runner_->RunsTasksInCurrentSequence());
  PostBackgroundTask(FROM_HERE,
                     base::BindOnce(&Backend::FlushAndNotifyInBackground, this,
                                    std::move(callback)));
}
*/

// Fire off a close message to the background runner.  We could still have a
// pending commit timer or Load operations holding references on us, but if/when
// this fires we will already have been cleaned up and it will be ignored.
void SQLitePersistentOriginManifestStore::Backend::Close(
    const base::Closure& callback) {
  if (background_task_runner_->RunsTasksInCurrentSequence()) {
    InternalBackgroundClose(callback);
  } else {
    // Must close the backend on the background runner.
    PostBackgroundTask(FROM_HERE, base::Bind(&Backend::InternalBackgroundClose,
                                             this, callback));
  }
}

void SQLitePersistentOriginManifestStore::Backend::InternalBackgroundClose(
    const base::Closure& callback) {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());
  // Commit any pending operations
  Commit();

  meta_table_.Reset();
  db_.reset();

  // We're clean now.
  if (!callback.is_null())
    callback.Run();
}

void SQLitePersistentOriginManifestStore::Backend::DatabaseErrorCallback(
    int error,
    sql::Statement* stmt) {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());

  if (!sql::IsErrorCatastrophic(error))
    return;

  // TODO(shess): Running KillDatabase() multiple times should be
  // safe.
  if (corruption_detected_)
    return;

  corruption_detected_ = true;

  // Don't just do the close/delete here, as we are being called by |db| and
  // that seems dangerous.
  // TODO(shess): Consider just calling RazeAndClose() immediately.
  // db_ may not be safe to reset at this point, but RazeAndClose()
  // would cause the stack to unwind safely with errors.
  PostBackgroundTask(FROM_HERE, base::Bind(&Backend::KillDatabase, this));
}

void SQLitePersistentOriginManifestStore::Backend::KillDatabase() {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());

  if (db_) {
    // This Backend will now be in-memory only. In a future run we will recreate
    // the database. Hopefully things go better then!
    bool success = db_->RazeAndClose();
    VLOG(1) << "RazeAndClose " << success;
    // UMA_HISTOGRAM_BOOLEAN("OriginManifest.KillDatabaseResult", success);
    meta_table_.Reset();
    db_.reset();
  }
}

void SQLitePersistentOriginManifestStore::Backend::DeleteAllInList(
    const std::list<std::string>& origins) {
  if (origins.empty())
    return;

  if (background_task_runner_->RunsTasksInCurrentSequence()) {
    BackgroundDeleteAllInList(origins);
  } else {
    // Perform deletion on background task runner.
    PostBackgroundTask(
        FROM_HERE,
        base::Bind(&Backend::BackgroundDeleteAllInList, this, origins));
  }
}

/*
void
SQLitePersistentOriginManifestStore::Backend::DeleteSessionCookiesOnStartup() {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());
  base::Time start_time = base::Time::Now();
  if (!db_->Execute("DELETE FROM cookies WHERE persistent != 1"))
    LOG(WARNING) << "Unable to delete session cookies.";

  UMA_HISTOGRAM_TIMES("Cookie.Startup.TimeSpentDeletingCookies",
                      base::Time::Now() - start_time);
  UMA_HISTOGRAM_COUNTS_1M("Cookie.Startup.NumberOfCookiesDeleted",
                          db_->GetLastChangeCount());
}
*/

void SQLitePersistentOriginManifestStore::Backend::BackgroundDeleteAllInList(
    const std::list<std::string>& origins) {
  DCHECK(background_task_runner_->RunsTasksInCurrentSequence());

  if (!db_)
    return;

  // Force a commit of any pending writes before issuing deletes.
  // TODO(rohitrao): Remove the need for this Commit() by instead pruning the
  // list of pending operations. https://crbug.com/486742.
  Commit();

  sql::Statement del_smt(db_->GetCachedStatement(
      SQL_FROM_HERE, "DELETE FROM originmanifests WHERE origin=?"));
  if (!del_smt.is_valid()) {
    LOG(WARNING) << "Unable to delete origin manifests on shutdown.";
    return;
  }

  sql::Transaction transaction(db_.get());
  if (!transaction.Begin()) {
    LOG(WARNING) << "Unable to delete origin manifests on shutdown.";
    return;
  }

  for (const auto& origin : origins) {
    const GURL url(origin);
    if (!url.is_valid())
      continue;

    del_smt.Reset(true);
    del_smt.BindString(0, origin);
    if (!del_smt.Run())
      NOTREACHED() << "Could not delete a origin manifest from the DB.";
  }

  if (!transaction.Commit())
    LOG(WARNING) << "Unable to delete origin manifests on shutdown.";
}

void SQLitePersistentOriginManifestStore::Backend::PostBackgroundTask(
    const tracked_objects::Location& origin,
    base::OnceClosure task) {
  if (!background_task_runner_->PostTask(origin, std::move(task))) {
    LOG(WARNING) << "Failed to post task from " << origin.ToString()
                 << " to background_task_runner_.";
  }
}

void SQLitePersistentOriginManifestStore::Backend::PostClientTask(
    const tracked_objects::Location& origin,
    base::OnceClosure task) {
  if (!client_task_runner_->PostTask(origin, std::move(task))) {
    LOG(WARNING) << "Failed to post task from " << origin.ToString()
                 << " to client_task_runner_.";
  }
}

void SQLitePersistentOriginManifestStore::Backend::
    FinishedLoadingOriginManifests(const LoadedCallback& loaded_callback,
                                   bool success) {
  PostClientTask(FROM_HERE, base::Bind(&Backend::CompleteLoadInForeground, this,
                                       loaded_callback, success));
}

SQLitePersistentOriginManifestStore::SQLitePersistentOriginManifestStore(
    const base::FilePath& path,
    const scoped_refptr<base::SequencedTaskRunner>& client_task_runner,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner)
    : backend_(new Backend(path, client_task_runner, background_task_runner)) {}

void SQLitePersistentOriginManifestStore::DeleteAllInList(
    const std::list<std::string>& origins) {
  if (backend_)
    backend_->DeleteAllInList(origins);
}

void SQLitePersistentOriginManifestStore::Close(const base::Closure& callback) {
  if (backend_) {
    backend_->Close(callback);

    // We release our reference to the Backend, though it will probably still
    // have a reference if the background runner has not run
    // Backend::InternalBackgroundClose() yet.
    backend_ = nullptr;
  }
}

void SQLitePersistentOriginManifestStore::Load(
    const LoadedCallback& loaded_callback) {
  DCHECK(!loaded_callback.is_null());
  if (backend_)
    backend_->Load(loaded_callback);
  else
    loaded_callback.Run(std::vector<mojom::OriginManifestPtr>());
}

void SQLitePersistentOriginManifestStore::LoadOriginManifestForOrigin(
    const std::string& origin,
    const LoadedCallback& loaded_callback) {
  DCHECK(!loaded_callback.is_null());
  if (backend_)
    backend_->LoadOriginManifestForOrigin(origin, loaded_callback);
  else
    loaded_callback.Run(std::vector<mojom::OriginManifestPtr>());
}

void SQLitePersistentOriginManifestStore::AddOriginManifest(
    const mojom::OriginManifestPtr& om) {
  if (backend_)
    backend_->AddOriginManifest(om);
}

void SQLitePersistentOriginManifestStore::DeleteOriginManifest(
    const mojom::OriginManifestPtr& om) {
  if (backend_)
    backend_->DeleteOriginManifest(om);
}

/*
void SQLitePersistentOriginManifestStore::SetForceKeepSessionState() {
  // This store never discards session-only cookies, so this call has no effect.
}

void SQLitePersistentOriginManifestStore::SetBeforeFlushCallback(
    base::RepeatingClosure callback) {
  if (backend_)
    backend_->SetBeforeFlushCallback(std::move(callback));
}

void SQLitePersistentOriginManifestStore::Flush(base::OnceClosure callback) {
  if (backend_)
    backend_->Flush(std::move(callback));
}
*/

SQLitePersistentOriginManifestStore::~SQLitePersistentOriginManifestStore() {
  Close(base::Closure());
}

}  // namespace origin_manifest
