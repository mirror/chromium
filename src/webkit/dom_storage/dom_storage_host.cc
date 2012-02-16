// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/dom_storage/dom_storage_host.h"

#include "googleurl/src/gurl.h"
#include "webkit/dom_storage/dom_storage_area.h"
#include "webkit/dom_storage/dom_storage_context.h"
#include "webkit/dom_storage/dom_storage_namespace.h"
#include "webkit/dom_storage/dom_storage_types.h"

namespace dom_storage {

DomStorageHost::DomStorageHost(DomStorageContext* context)
    : last_connection_id_(0),
      context_(context) {
}

DomStorageHost::~DomStorageHost() {
  AreaMap::const_iterator it = connections_.begin();
  for (; it != connections_.end(); ++it)
    it->second.namespace_->CloseStorageArea(it->second.area_);
}

// TODO(michaeln): have the caller pass in the 'connection_id' value
// instead of generating them here, avoids a sync ipc on open.
int DomStorageHost::OpenStorageArea(int namespace_id, const GURL& origin) {
  NamespaceAndArea references;
  references.namespace_ = context_->GetStorageNamespace(namespace_id);
  if (!references.namespace_)
    return kInvalidAreaId;
  references.area_ = references.namespace_->OpenStorageArea(origin);
  if (!references.area_)
    return kInvalidAreaId;
  int id = ++last_connection_id_;
  connections_[id] = references;
  return id;
}

void DomStorageHost::CloseStorageArea(int connection_id) {
  AreaMap::iterator found = connections_.find(connection_id);
  if (found == connections_.end())
    return;
  found->second.namespace_->CloseStorageArea(
      found->second.area_);
  connections_.erase(found);
}

unsigned DomStorageHost::GetAreaLength(int connection_id) {
  DomStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return 0;
  return area->Length();
}

NullableString16 DomStorageHost::GetAreaKey(int connection_id, unsigned index) {
  DomStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return NullableString16(true);
  return area->Key(index);
}

NullableString16 DomStorageHost::GetAreaItem(int connection_id,
                                             const string16& key) {
  DomStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return NullableString16(true);
  return area->GetItem(key);
}

bool DomStorageHost::SetAreaItem(
    int connection_id, const string16& key,
    const string16& value, const GURL& page_url,
    NullableString16* old_value) {
  DomStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return false;
  if (!area->SetItem(key, value, old_value))
    return false;
  if ((area->namespace_id() == kLocalStorageNamespaceId) &&
      (old_value->is_null() || old_value->string() != value)) {
    context_->NotifyItemSet(area, key, value, *old_value, page_url);
  }
  return true;
}

bool DomStorageHost::RemoveAreaItem(
    int connection_id, const string16& key, const GURL& page_url,
    string16* old_value) {
  DomStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return false;
  if (!area->RemoveItem(key, old_value))
    return false;
  if (area->namespace_id() == kLocalStorageNamespaceId)
    context_->NotifyItemRemoved(area, key, *old_value, page_url);
  return true;
}

bool DomStorageHost::ClearArea(int connection_id, const GURL& page_url) {
  DomStorageArea* area = GetOpenArea(connection_id);
  if (!area)
    return false;
  if (!area->Clear())
    return false;
  if (area->namespace_id() == kLocalStorageNamespaceId)
    context_->NotifyAreaCleared(area, page_url);
  return true;
}

DomStorageArea* DomStorageHost::GetOpenArea(int connection_id) {
  AreaMap::iterator found = connections_.find(connection_id);
  if (found == connections_.end())
    return NULL;
  return found->second.area_;
}

// NamespaceAndArea

DomStorageHost::NamespaceAndArea::NamespaceAndArea() {}
DomStorageHost::NamespaceAndArea::~NamespaceAndArea() {}

}  // namespace dom_storage
