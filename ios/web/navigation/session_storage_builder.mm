// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/session_storage_builder.h"

#include <memory>

#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/web/navigation/crw_session_controller+private_constructors.h"
#import "ios/web/navigation/crw_session_controller.h"
#import "ios/web/navigation/navigation_item_impl.h"
#import "ios/web/navigation/navigation_item_storage_builder.h"
#include "ios/web/navigation/navigation_manager_impl.h"
#import "ios/web/public/crw_session_storage.h"
#import "ios/web/public/serializable_user_data_manager.h"
#import "ios/web/web_state/session_certificate_policy_cache_impl.h"
#include "ios/web/web_state/session_certificate_policy_cache_storage_builder.h"
#import "ios/web/web_state/web_state_impl.h"
#include "net/base/escape.h"

// CRWSessionController's readonly properties redefined as readwrite.  These
// will be removed and NavigationManagerImpl's ivars will be written directly
// as this functionality moves from CRWSessionController to
// NavigationManagerImpl;
@interface CRWSessionController (ExposedForSerialization)
@property(nonatomic, readwrite, assign) NSInteger previousItemIndex;
@end

namespace web {

CRWSessionStorage* SessionStorageBuilder::BuildStorage(
    WebStateImpl* web_state) const {
  DCHECK(web_state);
  web::NavigationManagerNewImpl* navigation_manager =
      web_state->navigation_manager_.get();
  DCHECK(navigation_manager);
  CRWSessionStorage* session_storage = [[CRWSessionStorage alloc] init];
  session_storage.hasOpener = web_state->HasOpener();
  session_storage.lastCommittedItemIndex =
      navigation_manager->GetLastCommittedItemIndex();

  // TODO(danyao): AFAICT, this is not used
  // session_storage.previousItemIndex = 0;

  NSMutableArray* item_storages = [[NSMutableArray alloc] init];
  NavigationItemStorageBuilder item_storage_builder;
  for (size_t index = 0; index < (size_t)navigation_manager->GetItemCount();
       index++) {
    NavigationItem* item = navigation_manager->GetItemAtIndex(index);
    [item_storages addObject:item_storage_builder.BuildStorage(item)];
  }

  session_storage.itemStorages = item_storages;
  SessionCertificatePolicyCacheStorageBuilder cert_builder;
  session_storage.certPolicyCacheStorage = cert_builder.BuildStorage(
      &web_state->GetSessionCertificatePolicyCacheImpl());
  web::SerializableUserDataManager* user_data_manager =
      web::SerializableUserDataManager::FromWebState(web_state);

  [session_storage
      setSerializableUserData:user_data_manager->CreateSerializableUserData()];
  return session_storage;
}

void SessionStorageBuilder::ExtractSessionState(
    WebStateImpl* web_state,
    CRWSessionStorage* storage) const {
  DCHECK(web_state);
  DCHECK(storage);
  web_state->created_with_opener_ = storage.hasOpener;

  // TODO(danyao): complete navigation restoration
  /*NSArray* item_storages = storage.itemStorages;
  web::ScopedNavigationItemList items(item_storages.count);

  for (size_t index = 0; index < item_storages.count; ++index) {
    std::unique_ptr<NavigationItemImpl> item_impl =
        item_storage_builder.BuildNavigationItemImpl(item_storages[index]);
    items[index] = std::move(item_impl);
  }
  NSUInteger last_committed_item_index = storage.lastCommittedItemIndex;*/

  web_state->navigation_manager_.reset(new NavigationManagerNewImpl());

  base::DictionaryValue sessionHistory;
  base::Value::ListStorage item_list;

  NSArray* item_storages = storage.itemStorages;
  NavigationItemStorageBuilder item_storage_builder;
  for (size_t index = 0; index < item_storages.count; ++index) {
    base::DictionaryValue item;
    std::unique_ptr<NavigationItemImpl> item_impl =
        item_storage_builder.BuildNavigationItemImpl(item_storages[index]);
    item.SetString("title", item_impl->GetTitle());
    item.SetString("url", item_impl->GetURL().spec());
    item_list.push_back(item);
  }
  sessionHistory.SetInteger(
      "currentItemOffset",
      storage.lastCommittedItemIndex - item_storages.count + 1);

  // NOTE(danyao): build fake session history
  /*
  base::DictionaryValue item1, item2, item3;
  item1.SetString("title", "Google");
  item1.SetString("url", "https://www.google.com");
  item2.SetString("title", "Amazon");
  item2.SetString("url", "https://amazon.com");
  item3.SetString("title", "Apple");
  item3.SetString("url", "https://apple.com");
  sessionHistory.SetInteger("currentItemOffset", -1);
  item_list.push_back(item1);
  item_list.push_back(item2);
  item_list.push_back(item3);
   */

  sessionHistory.Set("entries", base::MakeUnique<base::ListValue>(item_list));

  std::string json_string;
  JSONStringValueSerializer serializer(&json_string);
  serializer.Serialize(sessionHistory);

  web_state->navigation_manager_->restoredSessionHistory =
      net::EscapeQueryParamValue(json_string, false);
  web_state->navigation_manager_->history_is_restored = item_list.size() > 0;

  SessionCertificatePolicyCacheStorageBuilder cert_builder;
  std::unique_ptr<SessionCertificatePolicyCacheImpl> cert_policy_cache =
      cert_builder.BuildSessionCertificatePolicyCache(
          storage.certPolicyCacheStorage);
  if (!cert_policy_cache)
    cert_policy_cache = base::MakeUnique<SessionCertificatePolicyCacheImpl>();
  web_state->certificate_policy_cache_ = std::move(cert_policy_cache);
  web::SerializableUserDataManager::FromWebState(web_state)
      ->AddSerializableUserData(storage.userData);
}

}  // namespace web
