// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/prefs/overlay_user_pref_store.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/values.h"

OverlayUserPrefStore::OverlayUserPrefStore(
    PersistentPrefStore* underlay)
    : underlay_(underlay) {
  underlay_->AddObserver(this);
}

bool OverlayUserPrefStore::IsSetInOverlay(const std::string& key) const {
  return overlay_.GetValue(key, NULL);
}

void OverlayUserPrefStore::AddObserver(PrefStore::Observer* observer) {
  observers_.AddObserver(observer);
}

void OverlayUserPrefStore::RemoveObserver(PrefStore::Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool OverlayUserPrefStore::HasObservers() const {
  return observers_.might_have_observers();
}

bool OverlayUserPrefStore::IsInitializationComplete() const {
  return underlay_->IsInitializationComplete();
}

bool OverlayUserPrefStore::GetValue(const std::string& key,
                                    const base::Value** result) const {
  // If the |key| shall NOT be stored in the overlay store, there must not
  // be an entry.
  DCHECK(ShallBeStoredInOverlay(key) || !overlay_.GetValue(key, NULL));

  if (overlay_.GetValue(key, result))
    return true;
  return underlay_->GetValue(key, result);
}

namespace {
// Similar to base::DictionaryValue::MergeDictionary(), except moves values from
// |source| to |target| instead of copying them.
void MergeDictionary(base::DictionaryValue* target,
                     base::DictionaryValue* source) {
  CHECK(source->is_dict());
  for (auto& pair : *source) {
    std::unique_ptr<base::Value> merge_value = std::move(pair.second);
    // Check whether we have to merge dictionaries.
    if (merge_value->IsType(base::Value::Type::DICTIONARY)) {
      base::DictionaryValue* sub_dict;
      if (target->GetDictionaryWithoutPathExpansion(pair.first, &sub_dict)) {
        MergeDictionary(sub_dict,
                        static_cast<base::DictionaryValue*>(merge_value.get()));
        continue;
      }
    }
    // All other cases: Move and hook it up.
    target->SetWithoutPathExpansion(pair.first, std::move(merge_value));
  }
}
}  // namespace

std::unique_ptr<base::DictionaryValue> OverlayUserPrefStore::GetValues() const {
  auto values = underlay_->GetValues();
  auto overlay_values = overlay_.AsDictionaryValue();
  MergeDictionary(values.get(), overlay_values.get());
  return values;
}

bool OverlayUserPrefStore::GetMutableValue(const std::string& key,
                                           base::Value** result) {
  if (!ShallBeStoredInOverlay(key))
    return underlay_->GetMutableValue(key, result);

  if (overlay_.GetValue(key, result))
    return true;

  // Try to create copy of underlay if the overlay does not contain a value.
  base::Value* underlay_value = NULL;
  if (!underlay_->GetMutableValue(key, &underlay_value))
    return false;

  *result = underlay_value->DeepCopy();
  overlay_.SetValue(key, base::WrapUnique(*result));
  return true;
}

void OverlayUserPrefStore::SetValue(const std::string& key,
                                    std::unique_ptr<base::Value> value,
                                    uint32_t flags) {
  if (!ShallBeStoredInOverlay(key)) {
    underlay_->SetValue(key, std::move(value), flags);
    return;
  }

  if (overlay_.SetValue(key, std::move(value)))
    ReportValueChanged(key, flags);
}

void OverlayUserPrefStore::SetValueSilently(const std::string& key,
                                            std::unique_ptr<base::Value> value,
                                            uint32_t flags) {
  if (!ShallBeStoredInOverlay(key)) {
    underlay_->SetValueSilently(key, std::move(value), flags);
    return;
  }

  overlay_.SetValue(key, std::move(value));
}

void OverlayUserPrefStore::RemoveValue(const std::string& key, uint32_t flags) {
  if (!ShallBeStoredInOverlay(key)) {
    underlay_->RemoveValue(key, flags);
    return;
  }

  if (overlay_.RemoveValue(key))
    ReportValueChanged(key, flags);
}

bool OverlayUserPrefStore::ReadOnly() const {
  return false;
}

PersistentPrefStore::PrefReadError OverlayUserPrefStore::GetReadError() const {
  return PersistentPrefStore::PREF_READ_ERROR_NONE;
}

PersistentPrefStore::PrefReadError OverlayUserPrefStore::ReadPrefs() {
  // We do not read intentionally.
  OnInitializationCompleted(true);
  return PersistentPrefStore::PREF_READ_ERROR_NONE;
}

void OverlayUserPrefStore::ReadPrefsAsync(
    ReadErrorDelegate* error_delegate_raw) {
  std::unique_ptr<ReadErrorDelegate> error_delegate(error_delegate_raw);
  // We do not read intentionally.
  OnInitializationCompleted(true);
}

void OverlayUserPrefStore::CommitPendingWrite() {
  underlay_->CommitPendingWrite();
  // We do not write our content intentionally.
}

void OverlayUserPrefStore::SchedulePendingLossyWrites() {
  underlay_->SchedulePendingLossyWrites();
}

void OverlayUserPrefStore::ReportValueChanged(const std::string& key,
                                              uint32_t flags) {
  for (PrefStore::Observer& observer : observers_)
    observer.OnPrefValueChanged(key);
}

void OverlayUserPrefStore::OnPrefValueChanged(const std::string& key) {
  if (!overlay_.GetValue(key, NULL))
    ReportValueChanged(key, DEFAULT_PREF_WRITE_FLAGS);
}

void OverlayUserPrefStore::OnInitializationCompleted(bool succeeded) {
  for (PrefStore::Observer& observer : observers_)
    observer.OnInitializationCompleted(succeeded);
}

void OverlayUserPrefStore::RegisterUnderlayPref(const std::string& key) {
  DCHECK(!key.empty()) << "Key is empty";
  DCHECK(underlay_names_set_.find(key) == underlay_names_set_.end())
      << "Key already registered";
  underlay_names_set_.insert(key);
}

void OverlayUserPrefStore::ClearMutableValues() {
  overlay_.Clear();
}

OverlayUserPrefStore::~OverlayUserPrefStore() {
  underlay_->RemoveObserver(this);
}

bool OverlayUserPrefStore::ShallBeStoredInOverlay(
    const std::string& key) const {
  return underlay_names_set_.find(key) == underlay_names_set_.end();
}
