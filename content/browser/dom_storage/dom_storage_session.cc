// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/dom_storage_session.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "content/browser/dom_storage/dom_storage_context_impl.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/dom_storage/dom_storage_task_runner.h"
#include "content/browser/dom_storage/session_storage_context_mojo.h"

namespace content {

namespace {
void CloneSessionNamespaceHelper(
    scoped_refptr<DOMStorageContextWrapper> context_wrapper,
    SessionStorageContextMojo* mojo_context,
    int64_t namespace_id_to_clone,
    int64_t clone_namespace_id,
    const std::string& clone_persistent_namespace_id) {
  mojo_context->CloneSessionNamespace(namespace_id_to_clone, clone_namespace_id,
                                      clone_persistent_namespace_id);
}

}  // namespace

// Constructed on thread that constructs DOMStorageSession. Used & destroyed on
// the mojo task runner that the |mojo_context_| runs on.
class DOMStorageSession::SequenceHelper {
 public:
  SequenceHelper(SessionStorageContextMojo* mojo_context)
      : mojo_context_(mojo_context) {}
  ~SequenceHelper() = default;

  void CreateSessionNamespace(int64_t namespace_id,
                              const std::string& persistent_namespace_id) {
    mojo_context_->CreateSessionNamespace(namespace_id,
                                          persistent_namespace_id);
  }

  void DeleteSessionNamespace(int64_t namespace_id, bool should_persist) {
    mojo_context_->DeleteSessionNamespace(namespace_id, should_persist);
  }

 private:
  SessionStorageContextMojo* mojo_context_;
};

DOMStorageSession::DOMStorageSession(
    scoped_refptr<DOMStorageContextImpl> context)
    : context_(std::move(context)),
      namespace_id_(context_->AllocateSessionId()),
      persistent_namespace_id_(context->AllocatePersistentSessionId()),
      should_persist_(false) {
  context_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DOMStorageContextImpl::CreateSessionNamespace, context_,
                     namespace_id_, persistent_namespace_id_));
}

DOMStorageSession::DOMStorageSession(
    scoped_refptr<DOMStorageContextWrapper> context)
    : mojo_context_(std::move(context)),
      mojo_task_runner_(mojo_context_->mojo_task_runner()),
      namespace_id_(mojo_context_->context()->AllocateSessionId()),
      persistent_namespace_id_(
          mojo_context_->context()->AllocatePersistentSessionId()),
      should_persist_(false),
      sequence_helper_(std::make_unique<SequenceHelper>(
          mojo_context_->mojo_session_state())) {
  mojo_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&SequenceHelper::CreateSessionNamespace,
                                base::Unretained(sequence_helper_.get()),
                                namespace_id_, persistent_namespace_id_));
}

DOMStorageSession::DOMStorageSession(
    scoped_refptr<DOMStorageContextImpl> context,
    const std::string& persistent_namespace_id)
    : context_(std::move(context)),
      namespace_id_(context->AllocateSessionId()),
      persistent_namespace_id_(persistent_namespace_id),
      should_persist_(false) {
  context_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DOMStorageContextImpl::CreateSessionNamespace, context_,
                     namespace_id_, persistent_namespace_id_));
}

DOMStorageSession::DOMStorageSession(
    scoped_refptr<DOMStorageContextWrapper> context,
    const std::string& persistent_namespace_id)
    : mojo_context_(std::move(context)),
      mojo_task_runner_(mojo_context_->mojo_task_runner()),
      namespace_id_(mojo_context_->context()->AllocateSessionId()),
      persistent_namespace_id_(persistent_namespace_id),
      should_persist_(false),
      sequence_helper_(std::make_unique<SequenceHelper>(
          mojo_context_->mojo_session_state())) {
  mojo_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&SequenceHelper::CreateSessionNamespace,
                                base::Unretained(sequence_helper_.get()),
                                namespace_id_, persistent_namespace_id_));
}

void DOMStorageSession::SetShouldPersist(bool should_persist) {
  should_persist_ = should_persist;
}

bool DOMStorageSession::should_persist() const {
  return should_persist_;
}

bool DOMStorageSession::IsFromContext(DOMStorageContextImpl* context) {
  return context_.get() == context;
}

std::unique_ptr<DOMStorageSession> DOMStorageSession::Clone() {
  return sequence_helper_ ? CloneFrom(mojo_context_, namespace_id_)
                          : CloneFrom(context_, namespace_id_);
}

// static
std::unique_ptr<DOMStorageSession> DOMStorageSession::CloneFrom(
    scoped_refptr<DOMStorageContextImpl> context,
    int64_t namespace_id_to_clone) {
  int64_t clone_id = context->AllocateSessionId();
  std::string persistent_clone_id = context->AllocatePersistentSessionId();
  context->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DOMStorageContextImpl::CloneSessionNamespace, context,
                     namespace_id_to_clone, clone_id, persistent_clone_id));
  return std::unique_ptr<DOMStorageSession>(
      new DOMStorageSession(std::move(context), clone_id, persistent_clone_id));
}

// static
std::unique_ptr<DOMStorageSession> DOMStorageSession::CloneFrom(
    scoped_refptr<DOMStorageContextWrapper> context,
    int64_t namespace_id_to_clone) {
  int64_t clone_id = context->context()->AllocateSessionId();
  std::string persistent_clone_id =
      context->context()->AllocatePersistentSessionId();
  context->mojo_task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CloneSessionNamespaceHelper, context,
                     context->mojo_session_state(), namespace_id_to_clone,
                     clone_id, persistent_clone_id));
  return std::unique_ptr<DOMStorageSession>(
      new DOMStorageSession(std::move(context), clone_id, persistent_clone_id));
}

DOMStorageSession::DOMStorageSession(
    scoped_refptr<DOMStorageContextImpl> context,
    int64_t namespace_id,
    const std::string& persistent_namespace_id)
    : context_(std::move(context)),
      namespace_id_(namespace_id),
      persistent_namespace_id_(persistent_namespace_id),
      should_persist_(false) {
  // This ctor is intended for use by the Clone() method.
}

DOMStorageSession::DOMStorageSession(
    scoped_refptr<DOMStorageContextWrapper> context,
    int64_t namespace_id,
    const std::string& persistent_namespace_id)
    : mojo_context_(std::move(context)),
      mojo_task_runner_(mojo_context_->mojo_task_runner()),
      namespace_id_(namespace_id),
      persistent_namespace_id_(persistent_namespace_id),
      should_persist_(false) {
  // This ctor is intended for use by the Clone() method.
}

DOMStorageSession::~DOMStorageSession() {
  DCHECK(!!context_ || !!sequence_helper_);
  if (context_) {
    context_->task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&DOMStorageContextImpl::DeleteSessionNamespace, context_,
                       namespace_id_, should_persist_));
  }
  if (sequence_helper_) {
    mojo_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&SequenceHelper::DeleteSessionNamespace,
                                  base::Unretained(sequence_helper_.get()),
                                  namespace_id_, should_persist_));
    mojo_task_runner_->DeleteSoon(FROM_HERE, std::move(sequence_helper_));
    mojo_context_->AddRef();
    if (!mojo_task_runner_->ReleaseSoon(FROM_HERE, mojo_context_.get()))
      mojo_context_->Release();
  }
}

}  // namespace content
