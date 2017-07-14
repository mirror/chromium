// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/state_store.h"

#include <stddef.h>

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/value_store/value_store_factory.h"
#include "extensions/common/extension.h"

namespace {

// Delay, in seconds, before we should open the State Store database. We
// defer it to avoid slowing down startup. See http://crbug.com/161848
const int kInitDelaySeconds = 1;

std::string GetFullKey(const std::string& extension_id,
                       const std::string& key) {
  return extension_id + "." + key;
}

}  // namespace

namespace extensions {

// Helper class to delay tasks until we're ready to start executing them.
class StateStore::DelayedTaskQueue {
 public:
  DelayedTaskQueue() : ready_(false) {}
  ~DelayedTaskQueue() {}

  // Queues up a task for invoking once we're ready. Invokes immediately if
  // we're already ready.
  void InvokeWhenReady(base::OnceClosure task);

  // Marks us ready, and invokes all pending tasks.
  void SetReady();

  // Return whether or not the DelayedTaskQueue is |ready_|.
  bool ready() const { return ready_; }

 private:
  bool ready_;
  std::vector<base::OnceClosure> pending_tasks_;

  DISALLOW_COPY_AND_ASSIGN(DelayedTaskQueue);
};

void StateStore::DelayedTaskQueue::InvokeWhenReady(base::OnceClosure task) {
  if (ready_) {
    std::move(task).Run();
  } else {
    pending_tasks_.push_back(std::move(task));
  }
}

void StateStore::DelayedTaskQueue::SetReady() {
  ready_ = true;

  for (size_t i = 0; i < pending_tasks_.size(); ++i)
    std::move(pending_tasks_[i]).Run();
  pending_tasks_.clear();
}

StateStore::StateStore(content::BrowserContext* context,
                       const scoped_refptr<ValueStoreFactory>& store_factory,
                       ValueStoreFrontend::BackendType backend_type,
                       bool deferred_load)
    : store_(new ValueStoreFrontend(store_factory, backend_type)),
      task_queue_(new DelayedTaskQueue()),
      extension_registry_observer_(this) {
  extension_registry_observer_.Add(ExtensionRegistry::Get(context));

  if (deferred_load) {
    // Don't Init() until the first page is loaded or the embedder explicitly
    // requests it.
    registrar_.Add(
        this,
        content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
        content::NotificationService::AllBrowserContextsAndSources());
  } else {
    Init();
  }
}

StateStore::~StateStore() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void StateStore::RequestInitAfterDelay() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  InitAfterDelay();
}

void StateStore::RegisterKey(const std::string& key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  registered_keys_.insert(key);
}

void StateStore::GetExtensionValue(const std::string& extension_id,
                                   const std::string& key,
                                   ReadCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  task_queue_->InvokeWhenReady(
      base::BindOnce(&ValueStoreFrontend::Get, base::Unretained(store_.get()),
                     GetFullKey(extension_id, key), std::move(callback)));
}

void StateStore::SetExtensionValue(const std::string& extension_id,
                                   const std::string& key,
                                   std::unique_ptr<base::Value> value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  task_queue_->InvokeWhenReady(
      base::BindOnce(&ValueStoreFrontend::Set, base::Unretained(store_.get()),
                     GetFullKey(extension_id, key), base::Passed(&value)));
}

void StateStore::RemoveExtensionValue(const std::string& extension_id,
                                      const std::string& key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  task_queue_->InvokeWhenReady(base::BindOnce(&ValueStoreFrontend::Remove,
                                              base::Unretained(store_.get()),
                                              GetFullKey(extension_id, key)));
}

bool StateStore::IsInitialized() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return task_queue_->ready();
}

void StateStore::Observe(int type,
                         const content::NotificationSource& source,
                         const content::NotificationDetails& details) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(type, content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME);
  registrar_.RemoveAll();
  InitAfterDelay();
}

void StateStore::OnExtensionWillBeInstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    bool is_update,
    const std::string& old_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  RemoveKeysForExtension(extension->id());
}

void StateStore::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    extensions::UninstallReason reason) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  RemoveKeysForExtension(extension->id());
}

void StateStore::Init() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Could be called twice if InitAfterDelay() is requested explicitly by the
  // embedder in addition to internally after first page load.
  if (IsInitialized())
    return;

  // TODO(cmumford): The store now always lazily initializes upon first access.
  // A follow-on CL will remove this deferred initialization implementation
  // which is now vestigial.
  task_queue_->SetReady();
}

void StateStore::InitAfterDelay() {
  if (IsInitialized())
    return;

  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&StateStore::Init, AsWeakPtr()),
      base::TimeDelta::FromSeconds(kInitDelaySeconds));
}

void StateStore::RemoveKeysForExtension(const std::string& extension_id) {
  for (std::set<std::string>::iterator key = registered_keys_.begin();
       key != registered_keys_.end();
       ++key) {
    task_queue_->InvokeWhenReady(base::BindOnce(
        &ValueStoreFrontend::Remove, base::Unretained(store_.get()),
        GetFullKey(extension_id, *key)));
  }
}

}  // namespace extensions
