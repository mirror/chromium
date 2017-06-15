// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/predictor_table_base.h"

#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "content/public/browser/browser_thread.h"
#include "sql/connection.h"

using content::BrowserThread;

namespace predictors {

scoped_refptr<base::SequencedTaskRunner> PredictorTableBase::GetTaskRunner() {
  return db_task_runner_;
}

PredictorTableBase::PredictorTableBase(
    scoped_refptr<base::SequencedTaskRunner> db_task_runner)
    : db_task_runner_(db_task_runner), db_(nullptr) {}

PredictorTableBase::~PredictorTableBase() {
}

void PredictorTableBase::Initialize(sql::Connection* db) {
  CHECK(db_task_runner_->RunsTasksInCurrentSequence());
  db_ = db;
  CreateTableIfNonExistent();
}

void PredictorTableBase::SetCancelled() {
  cancelled_.Set();
}

sql::Connection* PredictorTableBase::DB() {
  CHECK(db_task_runner_->RunsTasksInCurrentSequence());
  return db_;
}

void PredictorTableBase::ResetDB() {
  CHECK(db_task_runner_->RunsTasksInCurrentSequence());
  db_ = nullptr;
}

bool PredictorTableBase::CantAccessDatabase() {
  CHECK(db_task_runner_->RunsTasksInCurrentSequence());
  return cancelled_.IsSet() || !db_;
}

}  // namespace predictors
