// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/sequenced_object.h"

#include "media/base/sequenced_scoped_ref_ptr.h"

namespace media {

InternalWeakRef::InternalWeakRef() {}
InternalWeakRef::InternalWeakRef(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : impl_task_runner(std::move(task_runner)) {}
InternalWeakRef::InternalWeakRef(
    SequencedObjectBase* p,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : ptr(p), impl_task_runner(std::move(task_runner)) {}
InternalWeakRef::~InternalWeakRef() {}

SequencedObjectBase::SequencedObjectBase() {
  RegisterWeakRef(
      new InternalWeakRef(this, base::SequencedTaskRunnerHandle::Get()));
}

SequencedObjectBase::SequencedObjectBase(
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  RegisterWeakRef(new InternalWeakRef(this, std::move(task_runner)));
}

SequencedObjectBase::~SequencedObjectBase() {
  // Clear all weak refs.
  for (auto& ref : weak_refs_) {
    ref->ptr = nullptr;
  }
  weak_refs_.clear();
  // Note that we no longer know our task runner.  Of course, it should be
  // the current task runner.
}

InternalWeakRef* SequencedObjectBase::GetWeakRefFromHolder() const {
  return weak_refs_[0].get();
}

OwnerRecord::OwnerRecord(scoped_refptr<base::SequencedTaskRunner> impl)
    : impl_task_runner(std::move(impl)) {}

OwnerRecord::OwnerRecord(std::unique_ptr<SequencedObjectBase> t0,
                         scoped_refptr<base::SequencedTaskRunner> impl)
    : t(std::move(t0)), impl_task_runner(std::move(impl)) {}

OwnerRecord::~OwnerRecord() {
  // Post destruction of |t_|.
  // It would be nice if we couldn't DeleteSoon a SequencedObject, but
  // it seems to evade our detection.  So, for now, we skip wrapping this in
  // something that would hide it.
  if (t)
    impl_task_runner->DeleteSoon(FROM_HERE, std::move(t));
}
}  // namespace media
