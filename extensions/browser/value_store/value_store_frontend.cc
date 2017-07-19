// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/value_store/value_store_frontend.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/value_store/leveldb_value_store.h"
#include "extensions/browser/value_store/value_store.h"
#include "extensions/browser/value_store/value_store_factory.h"

using extensions::ValueStoreFactory;

class ValueStoreFrontend::Backend
    : public base::RefCountedDeleteOnSequence<Backend> {
 public:
  Backend(scoped_refptr<base::SequencedTaskRunner> task_runner,
          const scoped_refptr<ValueStoreFactory>& store_factory,
          BackendType backend_type)
      : base::RefCountedDeleteOnSequence<Backend>(std::move(task_runner)),
        store_factory_(store_factory),
        backend_type_(backend_type) {
    // The instance is not created on |task_runner|, but it should only be
    // accessed from it in the future.
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  std::unique_ptr<base::Value> Get(const std::string& key) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    LazyInit();
    ValueStore::ReadResult result = storage_->Get(key);

    // Extract the value from the ReadResult and return it.
    std::unique_ptr<base::Value> value;
    if (result->status().ok()) {
      result->settings().RemoveWithoutPathExpansion(key, &value);
    } else {
      LOG(WARNING) << "Reading " << key << " from " << db_path_.value()
                   << " failed: " << result->status().message;
    }

    return value;
  }

  void Set(const std::string& key, std::unique_ptr<base::Value> value) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    LazyInit();
    // We don't need the old value, so skip generating changes.
    ValueStore::WriteResult result = storage_->Set(
        ValueStore::IGNORE_QUOTA | ValueStore::NO_GENERATE_CHANGES, key,
        *value);
    LOG_IF(ERROR, !result->status().ok()) << "Error while writing " << key
                                          << " to " << db_path_.value();
  }

  void Remove(const std::string& key) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    LazyInit();
    storage_->Remove(key);
  }

 private:
  friend class base::RefCountedDeleteOnSequence<Backend>;
  friend class base::DeleteHelper<Backend>;

  virtual ~Backend() { DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_); }

  void LazyInit() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    if (storage_)
      return;
    TRACE_EVENT0("ValueStoreFrontend::Backend", "LazyInit");
    switch (backend_type_) {
      case BackendType::RULES:
        storage_ = store_factory_->CreateRulesStore();
        break;
      case BackendType::STATE:
        storage_ = store_factory_->CreateStateStore();
        break;
    }
  }

  // The factory which will be used to lazily create the ValueStore when needed.
  // Used exclusively on the FILE thread.
  scoped_refptr<ValueStoreFactory> store_factory_;
  BackendType backend_type_;

  // The actual ValueStore that handles persisting the data to disk. Used
  // exclusively on the FILE thread.
  std::unique_ptr<ValueStore> storage_;

  base::FilePath db_path_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(Backend);
};

ValueStoreFrontend::ValueStoreFrontend(
    const scoped_refptr<ValueStoreFactory>& store_factory,
    BackendType backend_type)
    : task_runner_(
          base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()})),
      backend_(new Backend(task_runner_, store_factory, backend_type)) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

ValueStoreFrontend::~ValueStoreFrontend() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

void ValueStoreFrontend::Get(const std::string& key, ReadCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE,
      base::BindOnce(&ValueStoreFrontend::Backend::Get, backend_, key),
      std::move(callback));
}

void ValueStoreFrontend::Set(const std::string& key,
                             std::unique_ptr<base::Value> value) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&ValueStoreFrontend::Backend::Set, backend_,
                                key, std::move(value)));
}

void ValueStoreFrontend::Remove(const std::string& key) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&ValueStoreFrontend::Backend::Remove, backend_, key));
}
