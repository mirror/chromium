// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/reading_list/ios/reading_list_model_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "components/prefs/pref_service.h"
#include "components/reading_list/ios/reading_list_model_storage.h"
#include "components/reading_list/ios/reading_list_pref_names.h"
#include "url/gurl.h"

ReadingListModelImpl::ReadingListModelImpl()
    : ReadingListModelImpl(nullptr, nullptr) {}

ReadingListModelImpl::ReadingListModelImpl(
    std::unique_ptr<ReadingListModelStorage> storage,
    PrefService* pref_service)
    : unread_entry_count_(0),
      read_entry_count_(0),
      pref_service_(pref_service),
      has_unseen_(false),
      loaded_(false),
      weak_ptr_factory_(this) {
  DCHECK(CalledOnValidThread());
  if (storage) {
    storage_layer_ = std::move(storage);
    storage_layer_->SetReadingListModel(this, this);
  } else {
    loaded_ = true;
    entries_ = base::MakeUnique<ReadingListEntries>();
  }
  has_unseen_ = GetPersistentHasUnseen();
}

ReadingListModelImpl::~ReadingListModelImpl() {}

void ReadingListModelImpl::StoreLoaded(
    std::unique_ptr<ReadingListEntries> entries) {
  DCHECK(CalledOnValidThread());
  entries_ = std::move(entries);
  for (auto& iterator : *entries_) {
    if (iterator.second.IsRead()) {
      read_entry_count_++;
    } else {
      unread_entry_count_++;
    }
  }
  DCHECK(read_entry_count_ + unread_entry_count_ == entries_->size());
  loaded_ = true;
  for (auto& observer : observers_)
    observer.ReadingListModelLoaded(this);
}

void ReadingListModelImpl::Shutdown() {
  DCHECK(CalledOnValidThread());
  for (auto& observer : observers_)
    observer.ReadingListModelBeingDeleted(this);
  loaded_ = false;
}

bool ReadingListModelImpl::loaded() const {
  DCHECK(CalledOnValidThread());
  return loaded_;
}

size_t ReadingListModelImpl::size() const {
  DCHECK(CalledOnValidThread());
  DCHECK(read_entry_count_ + unread_entry_count_ == entries_->size());
  if (!loaded())
    return 0;
  return entries_->size();
}

size_t ReadingListModelImpl::unread_size() const {
  DCHECK(CalledOnValidThread());
  DCHECK(read_entry_count_ + unread_entry_count_ == entries_->size());
  if (!loaded())
    return 0;
  return unread_entry_count_;
}

bool ReadingListModelImpl::HasUnseenEntries() const {
  DCHECK(CalledOnValidThread());
  if (!loaded())
    return false;
  return unread_entry_count_ > 0 && has_unseen_;
}

void ReadingListModelImpl::ResetUnseenEntries() {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  has_unseen_ = false;
  if (!IsPerformingBatchUpdates())
    SetPersistentHasUnseen(false);
}

const std::vector<GURL> ReadingListModelImpl::Keys() const {
  std::vector<GURL> keys;
  for (const auto& iterator : *entries_) {
    keys.push_back(iterator.first);
  }
  return keys;
}

const ReadingListEntry* ReadingListModelImpl::GetEntryByURL(
    const GURL& gurl) const {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  return GetMutableEntryFromURL(gurl);
}

ReadingListEntry* ReadingListModelImpl::GetMutableEntryFromURL(
    const GURL& url) const {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  auto iterator = entries_->find(url);
  if (iterator == entries_->end()) {
    return nullptr;
  }
  return &(iterator->second);
}

void ReadingListModelImpl::SyncAddEntry(
    std::unique_ptr<ReadingListEntry> entry) {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  // entry must not already exist.
  DCHECK(GetMutableEntryFromURL(entry->URL()) == nullptr);
  for (auto& observer : observers_)
    observer.ReadingListWillAddEntry(this, *entry);
  if (entry->IsRead()) {
    read_entry_count_++;
  } else {
    unread_entry_count_++;
    SetPersistentHasUnseen(true);
  }
  GURL url = entry->URL();
  entries_->insert(std::make_pair(url, std::move(*entry)));
  for (auto& observer : observers_) {
    observer.ReadingListDidAddEntry(this, url);
    observer.ReadingListDidApplyChanges(this);
  }
}

ReadingListEntry* ReadingListModelImpl::SyncMergeEntry(
    std::unique_ptr<ReadingListEntry> entry) {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  ReadingListEntry* existing_entry = GetMutableEntryFromURL(entry->URL());
  DCHECK(existing_entry);
  GURL url = entry->URL();

  for (auto& observer : observers_)
    observer.ReadingListWillMoveEntry(this, url);

  if (existing_entry->IsRead()) {
    read_entry_count_--;
  } else {
    unread_entry_count_--;
  }
  existing_entry->MergeWithEntry(*entry);

  existing_entry = GetMutableEntryFromURL(url);
  if (existing_entry->IsRead()) {
    read_entry_count_++;
  } else {
    unread_entry_count_++;
  }
  for (auto& observer : observers_) {
    observer.ReadingListDidMoveEntry(this, url);
    observer.ReadingListDidApplyChanges(this);
  }

  return existing_entry;
}

void ReadingListModelImpl::SyncRemoveEntry(const GURL& url) {
  RemoveEntryByURLImpl(url, true);
}

void ReadingListModelImpl::RemoveEntryByURL(const GURL& url) {
  RemoveEntryByURLImpl(url, false);
}

void ReadingListModelImpl::RemoveEntryByURLImpl(const GURL& url,
                                                bool from_sync) {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  const ReadingListEntry* entry = GetEntryByURL(url);
  if (!entry)
    return;

  for (auto& observer : observers_)
    observer.ReadingListWillRemoveEntry(this, url);

  if (storage_layer_ && !from_sync) {
    storage_layer_->RemoveEntry(*entry);
  }
  if (entry->IsRead()) {
    read_entry_count_--;
  } else {
    unread_entry_count_--;
  }
  entries_->erase(url);
  for (auto& observer : observers_)
    observer.ReadingListDidApplyChanges(this);
}

const ReadingListEntry& ReadingListModelImpl::AddEntry(
    const GURL& url,
    const std::string& title) {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  RemoveEntryByURL(url);

  std::string trimmedTitle(title);
  base::TrimWhitespaceASCII(trimmedTitle, base::TRIM_ALL, &trimmedTitle);

  ReadingListEntry entry(url, trimmedTitle);
  for (auto& observer : observers_)
    observer.ReadingListWillAddEntry(this, entry);
  has_unseen_ = true;
  SetPersistentHasUnseen(true);
  entries_->insert(std::make_pair(url, std::move(entry)));
  unread_entry_count_++;
  if (storage_layer_) {
    storage_layer_->SaveEntry(*GetEntryByURL(url));
  }

  for (auto& observer : observers_) {
    observer.ReadingListDidAddEntry(this, url);
    observer.ReadingListDidApplyChanges(this);
  }

  return entries_->at(url);
}

void ReadingListModelImpl::SetReadStatus(const GURL& url, bool read) {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  auto iterator = entries_->find(url);
  if (iterator == entries_->end()) {
    return;
  }
  ReadingListEntry& entry = iterator->second;
  if (entry.IsRead() == read) {
    return;
  }
  for (ReadingListModelObserver& observer : observers_) {
    observer.ReadingListWillMoveEntry(this, url);
  }
  if (read) {
    read_entry_count_++;
    unread_entry_count_--;
  } else {
    unread_entry_count_++;
    read_entry_count_--;
  }
  entry.SetRead(read);
  entry.MarkEntryUpdated();
  if (storage_layer_) {
    storage_layer_->SaveEntry(entry);
  }
  for (ReadingListModelObserver& observer : observers_) {
    observer.ReadingListDidMoveEntry(this, url);
    observer.ReadingListDidApplyChanges(this);
  }
}

void ReadingListModelImpl::SetEntryTitle(const GURL& url,
                                         const std::string& title) {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  auto iterator = entries_->find(url);
  if (iterator == entries_->end()) {
    return;
  }
  ReadingListEntry& entry = iterator->second;
  if (entry.Title() == title) {
    return;
  }

  for (ReadingListModelObserver& observer : observers_) {
    observer.ReadingListWillUpdateEntry(this, url);
  }
  entry.SetTitle(title);
  if (storage_layer_) {
    storage_layer_->SaveEntry(entry);
  }
  for (ReadingListModelObserver& observer : observers_) {
    observer.ReadingListDidApplyChanges(this);
  }
}

void ReadingListModelImpl::SetEntryDistilledPath(
    const GURL& url,
    const base::FilePath& distilled_path) {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  auto iterator = entries_->find(url);
  if (iterator == entries_->end()) {
    return;
  }
  ReadingListEntry& entry = iterator->second;
  if (entry.DistilledState() == ReadingListEntry::PROCESSED &&
      entry.DistilledPath() == distilled_path) {
    return;
  }

  for (ReadingListModelObserver& observer : observers_) {
    observer.ReadingListWillUpdateEntry(this, url);
  }
  entry.SetDistilledPath(distilled_path);
  if (storage_layer_) {
    storage_layer_->SaveEntry(entry);
  }
  for (ReadingListModelObserver& observer : observers_) {
    observer.ReadingListDidApplyChanges(this);
  }
}

void ReadingListModelImpl::SetEntryDistilledState(
    const GURL& url,
    ReadingListEntry::DistillationState state) {
  DCHECK(CalledOnValidThread());
  DCHECK(loaded());
  auto iterator = entries_->find(url);
  if (iterator == entries_->end()) {
    return;
  }
  ReadingListEntry& entry = iterator->second;
  if (entry.DistilledState() == state) {
    return;
  }

  for (ReadingListModelObserver& observer : observers_) {
    observer.ReadingListWillUpdateEntry(this, url);
  }
  entry.SetDistilledState(state);
  if (storage_layer_) {
    storage_layer_->SaveEntry(entry);
  }
  for (ReadingListModelObserver& observer : observers_) {
    observer.ReadingListDidApplyChanges(this);
  }
}

std::unique_ptr<ReadingListModel::ScopedReadingListBatchUpdate>
ReadingListModelImpl::CreateBatchToken() {
  return base::MakeUnique<ReadingListModelImpl::ScopedReadingListBatchUpdate>(
      this);
}

ReadingListModelImpl::ScopedReadingListBatchUpdate::
    ScopedReadingListBatchUpdate(ReadingListModelImpl* model)
    : ReadingListModel::ScopedReadingListBatchUpdate::
          ScopedReadingListBatchUpdate(model) {
  if (model->StorageLayer()) {
    storage_token_ = model->StorageLayer()->EnsureBatchCreated();
  }
}

ReadingListModelImpl::ScopedReadingListBatchUpdate::
    ~ScopedReadingListBatchUpdate() {
  storage_token_.reset();
}

void ReadingListModelImpl::LeavingBatchUpdates() {
  DCHECK(CalledOnValidThread());
  if (storage_layer_) {
    SetPersistentHasUnseen(has_unseen_);
  }
  ReadingListModel::LeavingBatchUpdates();
}

void ReadingListModelImpl::EnteringBatchUpdates() {
  DCHECK(CalledOnValidThread());
  ReadingListModel::EnteringBatchUpdates();
}

void ReadingListModelImpl::SetPersistentHasUnseen(bool has_unseen) {
  DCHECK(CalledOnValidThread());
  if (!pref_service_) {
    return;
  }
  pref_service_->SetBoolean(reading_list::prefs::kReadingListHasUnseenEntries,
                            has_unseen);
}

bool ReadingListModelImpl::GetPersistentHasUnseen() {
  DCHECK(CalledOnValidThread());
  if (!pref_service_) {
    return false;
  }
  return pref_service_->GetBoolean(
      reading_list::prefs::kReadingListHasUnseenEntries);
}

syncer::ModelTypeSyncBridge* ReadingListModelImpl::GetModelTypeSyncBridge() {
  DCHECK(loaded());
  if (!storage_layer_)
    return nullptr;
  return storage_layer_.get();
}

ReadingListModelStorage* ReadingListModelImpl::StorageLayer() {
  return storage_layer_.get();
}
