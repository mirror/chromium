// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/settings_cookies_view_handler.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/i18n/number_formatting.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/browsing_data/browsing_data_appcache_helper.h"
#include "chrome/browser/browsing_data/browsing_data_cache_storage_helper.h"
#include "chrome/browser/browsing_data/browsing_data_channel_id_helper.h"
#include "chrome/browser/browsing_data/browsing_data_cookie_helper.h"
#include "chrome/browser/browsing_data/browsing_data_database_helper.h"
#include "chrome/browser/browsing_data/browsing_data_file_system_helper.h"
#include "chrome/browser/browsing_data/browsing_data_flash_lso_helper.h"
#include "chrome/browser/browsing_data/browsing_data_indexed_db_helper.h"
#include "chrome/browser/browsing_data/browsing_data_local_storage_helper.h"
#include "chrome/browser/browsing_data/browsing_data_media_license_helper.h"
#include "chrome/browser/browsing_data/browsing_data_quota_helper.h"
#include "chrome/browser/browsing_data/browsing_data_service_worker_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/cookies_tree_model_util.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/text/bytes_formatting.h"

namespace storage {
class FileSystemContext;
}

namespace {

int GetCategoryLabelID(CookieTreeNode::DetailedInfo::NodeType node_type) {
  const struct {
    CookieTreeNode::DetailedInfo::NodeType node_type;
    int id;
  } kCategoryLabels[] = {
      // The values (id) in this lookup table are up to UX. They may appear to
      // be
      // incorrect because multiple keys have the same value.
      {CookieTreeNode::DetailedInfo::TYPE_COOKIES,
       IDS_SETTINGS_COOKIES_SINGLE_COOKIE},
      {CookieTreeNode::DetailedInfo::TYPE_COOKIE,
       IDS_SETTINGS_COOKIES_SINGLE_COOKIE},

      {CookieTreeNode::DetailedInfo::TYPE_DATABASES,
       IDS_SETTINGS_COOKIES_DATABASE_STORAGE},
      {CookieTreeNode::DetailedInfo::TYPE_DATABASE,
       IDS_SETTINGS_COOKIES_DATABASE_STORAGE},

      {CookieTreeNode::DetailedInfo::TYPE_LOCAL_STORAGES,
       IDS_SETTINGS_COOKIES_LOCAL_STORAGE},
      {CookieTreeNode::DetailedInfo::TYPE_LOCAL_STORAGE,
       IDS_SETTINGS_COOKIES_LOCAL_STORAGE},

      {CookieTreeNode::DetailedInfo::TYPE_APPCACHES,
       IDS_SETTINGS_COOKIES_APPLICATION_CACHE},
      {CookieTreeNode::DetailedInfo::TYPE_APPCACHE,
       IDS_SETTINGS_COOKIES_APPLICATION_CACHE},

      {CookieTreeNode::DetailedInfo::TYPE_INDEXED_DBS,
       IDS_SETTINGS_COOKIES_DATABASE_STORAGE},
      {CookieTreeNode::DetailedInfo::TYPE_INDEXED_DB,
       IDS_SETTINGS_COOKIES_DATABASE_STORAGE},

      {CookieTreeNode::DetailedInfo::TYPE_FILE_SYSTEMS,
       IDS_SETTINGS_COOKIES_FILE_SYSTEM},
      {CookieTreeNode::DetailedInfo::TYPE_FILE_SYSTEM,
       IDS_SETTINGS_COOKIES_FILE_SYSTEM},

      {CookieTreeNode::DetailedInfo::TYPE_CHANNEL_IDS,
       IDS_SETTINGS_COOKIES_CHANNEL_ID},
      {CookieTreeNode::DetailedInfo::TYPE_CHANNEL_ID,
       IDS_SETTINGS_COOKIES_CHANNEL_ID},

      {CookieTreeNode::DetailedInfo::TYPE_SERVICE_WORKERS,
       IDS_SETTINGS_COOKIES_SERVICE_WORKER},
      {CookieTreeNode::DetailedInfo::TYPE_SERVICE_WORKER,
       IDS_SETTINGS_COOKIES_SERVICE_WORKER},

      {CookieTreeNode::DetailedInfo::TYPE_CACHE_STORAGES,
       IDS_SETTINGS_COOKIES_CACHE_STORAGE},
      {CookieTreeNode::DetailedInfo::TYPE_CACHE_STORAGE,
       IDS_SETTINGS_COOKIES_CACHE_STORAGE},

      {CookieTreeNode::DetailedInfo::TYPE_FLASH_LSO,
       IDS_SETTINGS_COOKIES_FLASH_LSO},

      {CookieTreeNode::DetailedInfo::TYPE_MEDIA_LICENSES,
       IDS_SETTINGS_COOKIES_MEDIA_LICENSE},
      {CookieTreeNode::DetailedInfo::TYPE_MEDIA_LICENSE,
       IDS_SETTINGS_COOKIES_MEDIA_LICENSE},
  };
  // Before optimizing, consider the data size and the cost of L2 cache misses.
  for (size_t i = 0; i < arraysize(kCategoryLabels); ++i) {
    if (kCategoryLabels[i].node_type == node_type) {
      return kCategoryLabels[i].id;
    }
  }
  NOTREACHED();
  return 0;
}

}  // namespace

namespace settings {

constexpr char kChildren[] = "children";
constexpr char kCount[] = "count";
// constexpr char kData[] = "data";
constexpr char kFilter[] = "filter";
constexpr char kId[] = "id";
constexpr char kItems[] = "item";
constexpr char kOrder[] = "order";
constexpr char kStart[] = "start";
// constexpr char kStatus[] = "status";
constexpr char kTotal[] = "total";

CookiesViewHandler::CookiesViewHandler()
    : batch_update_(false),
      is_local_data_request_(false),
      model_util_(new CookiesTreeModelUtil) {}

CookiesViewHandler::~CookiesViewHandler() {
}

void CookiesViewHandler::OnJavascriptAllowed() {
}

void CookiesViewHandler::OnJavascriptDisallowed() {
}

void CookiesViewHandler::RegisterMessages() {
  EnsureCookiesTreeModelCreated();

  web_ui()->RegisterMessageCallback(
      "localData.getDisplayList",
      base::Bind(&CookiesViewHandler::HandleGetDisplayList,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "localData.removeItem", base::Bind(&CookiesViewHandler::HandleRemoveItem,
                                         base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getCookieDetails",
      base::Bind(&CookiesViewHandler::HandleGetCookieDetails,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "updateCookieSearchResults",
      base::Bind(&CookiesViewHandler::HandleUpdateSearchResults,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "removeAllCookies",
      base::Bind(&CookiesViewHandler::HandleRemoveAll, base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "removeCookie",
      base::Bind(&CookiesViewHandler::HandleRemove, base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "loadCookie", base::Bind(&CookiesViewHandler::HandleLoadChildren,
                               base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "reloadCookies", base::Bind(&CookiesViewHandler::HandleReloadCookies,
                                  base::Unretained(this)));
}

void CookiesViewHandler::TreeNodesAdded(ui::TreeModel* model,
                                        ui::TreeModelNode* parent,
                                        int start,
                                        int count) {
  // Skip if there is a batch update in progress.
  if (batch_update_)
    return;

  CookiesTreeModel* tree_model = static_cast<CookiesTreeModel*>(model);
  CookieTreeNode* parent_node = tree_model->AsNode(parent);

  std::unique_ptr<base::ListValue> children(new base::ListValue);
  // Passing false for |include_quota_nodes| since they don't reflect reality
  // until bug http://crbug.com/642955 is fixed and local/session storage is
  // counted against the total.
  model_util_->GetChildNodeList(
      parent_node, start, count, /*include_quota_nodes=*/false, children.get());

  base::DictionaryValue args;
  if (parent == tree_model->GetRoot())
    args.Set(kId, base::MakeUnique<base::Value>());
  else
    args.SetString(kId, model_util_->GetTreeNodeId(parent_node));
  args.SetInteger(kStart, start);
  args.Set(kChildren, std::move(children));
  FireWebUIListener("onTreeItemAdded", args);
}

void CookiesViewHandler::TreeNodesRemoved(ui::TreeModel* model,
                                          ui::TreeModelNode* parent,
                                          int start,
                                          int count) {
  // Skip if there is a batch update in progress.
  if (batch_update_)
    return;

  CookiesTreeModel* tree_model = static_cast<CookiesTreeModel*>(model);

  base::DictionaryValue args;
  if (parent == tree_model->GetRoot())
    args.Set(kId, base::MakeUnique<base::Value>());
  else
    args.SetString(kId, model_util_->GetTreeNodeId(tree_model->AsNode(parent)));
  args.SetInteger(kStart, start);
  args.SetInteger(kCount, count);
  FireWebUIListener("onTreeItemRemoved", args);
}

void CookiesViewHandler::TreeModelBeginBatch(CookiesTreeModel* model) {
  DCHECK(!batch_update_);  // There should be no nested batch begin.
  batch_update_ = true;
}

void CookiesViewHandler::TreeModelEndBatch(CookiesTreeModel* model) {
  DCHECK(batch_update_);
  batch_update_ = false;

  if (IsJavascriptAllowed()) {
    if (is_local_data_request_)
      SendLocalDataList(model->GetRoot());
    else
      SendChildren(model->GetRoot());
  }
}

void CookiesViewHandler::EnsureCookiesTreeModelCreated() {
  if (!cookies_tree_model_.get()) {
    Profile* profile = Profile::FromWebUI(web_ui());
    content::StoragePartition* storage_partition =
        content::BrowserContext::GetDefaultStoragePartition(profile);
    content::IndexedDBContext* indexed_db_context =
        storage_partition->GetIndexedDBContext();
    content::ServiceWorkerContext* service_worker_context =
        storage_partition->GetServiceWorkerContext();
    content::CacheStorageContext* cache_storage_context =
        storage_partition->GetCacheStorageContext();
    storage::FileSystemContext* file_system_context =
        storage_partition->GetFileSystemContext();
    LocalDataContainer* container = new LocalDataContainer(
        new BrowsingDataCookieHelper(profile->GetRequestContext()),
        new BrowsingDataDatabaseHelper(profile),
        new BrowsingDataLocalStorageHelper(profile),
        /*session_storage_helper=*/nullptr,
        new BrowsingDataAppCacheHelper(profile),
        new BrowsingDataIndexedDBHelper(indexed_db_context),
        BrowsingDataFileSystemHelper::Create(file_system_context),
        BrowsingDataQuotaHelper::Create(profile),
        BrowsingDataChannelIDHelper::Create(profile->GetRequestContext()),
        new BrowsingDataServiceWorkerHelper(service_worker_context),
        new BrowsingDataCacheStorageHelper(cache_storage_context),
        BrowsingDataFlashLSOHelper::Create(profile),
        BrowsingDataMediaLicenseHelper::Create(file_system_context));
    cookies_tree_model_.reset(
        new CookiesTreeModel(container,
                             profile->GetExtensionSpecialStoragePolicy()));
    cookies_tree_model_->AddCookiesTreeObserver(this);
  }
}

void CookiesViewHandler::HandleUpdateSearchResults(
    const base::ListValue* args) {
  base::string16 query;
  CHECK(args->GetString(0, &query));

  cookies_tree_model_->UpdateSearchResults(query);
}

void CookiesViewHandler::HandleGetCookieDetails(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  CHECK(args->GetString(0, &callback_id_));
  std::string site;
  CHECK(args->GetString(1, &site));

  AllowJavascript();
  const CookieTreeNode* node = model_util_->GetTreeNodeFromTitle(
      cookies_tree_model_->GetRoot(), base::UTF8ToUTF16(site));

  if (!node) {
    RejectJavascriptCallback(base::Value(callback_id_), base::Value());
    callback_id_.clear();
    return;
  }

  SendCookieDetails(node);
}

void CookiesViewHandler::HandleGetDisplayList(const base::ListValue* args) {
  CHECK_EQ(3U, args->GetSize());
  CHECK(callback_id_.empty());
  CHECK(args->GetString(0, &callback_id_));
  int begin;
  CHECK(args->GetInteger(1, &begin));
  int count;
  CHECK(args->GetInteger(2, &count));

  AllowJavascript();
  is_local_data_request_ = true;
  local_data_request_start_ = begin;
  local_data_request_count_ = count;
  CHECK(local_data_request_start_ >= 0);
  CHECK(local_data_request_count_ >= 0);
  cookies_tree_model_.reset();
  EnsureCookiesTreeModelCreated();

  // Dave
}

void CookiesViewHandler::HandleReloadCookies(const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  CHECK(callback_id_.empty());
  CHECK(args->GetString(0, &callback_id_));

  AllowJavascript();
  cookies_tree_model_.reset();
  EnsureCookiesTreeModelCreated();
}

void CookiesViewHandler::HandleRemoveAll(const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  CHECK(args->GetString(0, &callback_id_));

  cookies_tree_model_->DeleteAllStoredObjects();
}

void CookiesViewHandler::HandleRemove(const base::ListValue* args) {
  std::string node_path;
  CHECK(args->GetString(0, &node_path));

  const CookieTreeNode* node = model_util_->GetTreeNodeFromPath(
      cookies_tree_model_->GetRoot(), node_path);
  if (node)
    cookies_tree_model_->DeleteCookieNode(const_cast<CookieTreeNode*>(node));
}

void CookiesViewHandler::HandleRemoveItem(const base::ListValue* args) {
  std::string item_id;
  CHECK(args->GetString(0, &item_id));

  const CookieTreeNode* node =
      model_util_->GetTreeNodeFromPath(cookies_tree_model_->GetRoot(), item_id);
  if (node)
    cookies_tree_model_->DeleteCookieNode(const_cast<CookieTreeNode*>(node));
}

void CookiesViewHandler::HandleLoadChildren(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  CHECK(args->GetString(0, &callback_id_));

  std::string node_path;
  CHECK(args->GetString(1, &node_path));

  const CookieTreeNode* node = model_util_->GetTreeNodeFromPath(
      cookies_tree_model_->GetRoot(), node_path);
  if (node)
    SendChildren(node);
}

void CookiesViewHandler::SendLocalDataList(const CookieTreeNode* parent) {
  is_local_data_request_ = false;
  int start = local_data_request_start_;
  int count = local_data_request_count_;
  int limit = std::min(start + count, parent->child_count());
  int parent_child_count = parent->child_count();
  // Check for integer wraparound/overflow. The client is not expected to know
  // the child_count, so this wraparound is not an error.
  if (limit < 0)
    limit = parent_child_count;

  ///*********************** Add timing UMA.
  // TODO(dschuyler): is it worthwhile (and practical) to avoid doing sorting
  // repeatedly (i.e. does it change to often to make a cache useful).

  // Sort the list by site.
  typedef std::pair<base::string16, int> ListItem;
  std::vector<ListItem> list_items;
  list_items.reserve(parent_child_count);  // Optimization, hint vector size.
  for (int i = 0; i < parent_child_count; ++i) {
    list_items.push_back(ListItem(parent->GetChild(i)->GetTitle(), i));
  }
  std::sort(list_items.begin(), list_items.end());

  // The layers in the CookieTree are:
  //   root - Top level.
  //   site - www.google.com, example.com, etc.
  //   category - Cookies, Channel ID, Local Storage, etc.
  //   item - Info on the actual thing.
  // Gather list of sites with some highlights of the categories and items.
  std::unique_ptr<base::ListValue> site_list(new base::ListValue);
  for (int i = start; i < limit; ++i) {
    const CookieTreeNode* site = parent->GetChild(list_items[i].second);
    std::string description;
    for (int k = 0; k < site->child_count(); ++k) {
      const CookieTreeNode* category = site->GetChild(k);
      int ids_value = GetCategoryLabelID(category->GetDetailedInfo().node_type);
      if (!ids_value) {
        // If we don't have a label to call it by, don't show it. Please add
        // a label ID if an expected category is not appearing in the UI.
        continue;
      }
      if (description.size()) {
        description += ", ";
      }
      int item_count = category->child_count();
      if (category->GetDetailedInfo().node_type ==
              CookieTreeNode::DetailedInfo::TYPE_COOKIES &&
          item_count > 1) {
        description +=
            l10n_util::GetStringFUTF8(IDS_SETTINGS_COOKIES_PLURAL_COOKIES,
                                      base::FormatNumber(item_count));
#if 0
      // TODO(dschuyler): is quota information indented for display?
      } else if (category->GetDetailedInfo().node_type ==
                 CookieTreeNode::DetailedInfo::TYPE_QUOTA) {
        const BrowsingDataQuotaHelper::QuotaInfo& quota_info =
            *category->GetDetailedInfo().quota_info;
        description += base::UTF16ToUTF8(ui::FormatBytes(
            quota_info.temporary_usage + quota_info.persistent_usage));
#endif
      } else {
        description += l10n_util::GetStringUTF8(ids_value);
      }
    }
    std::unique_ptr<base::DictionaryValue> list_info(new base::DictionaryValue);
    list_info->Set("subLabel", base::MakeUnique<base::Value>(description));
    std::string title = base::UTF16ToUTF8(site->GetTitle());
    list_info->Set("label", base::MakeUnique<base::Value>(title));
    site_list->Append(std::move(list_info));
  }

  base::DictionaryValue response;
  response.Set(kFilter, base::MakeUnique<base::Value>());
  response.Set(kItems, std::move(site_list));
  response.Set(kOrder, base::MakeUnique<base::Value>());
  response.Set(kStart, base::MakeUnique<base::Value>(start));
  response.Set(kTotal, base::MakeUnique<base::Value>(parent_child_count));

  ResolveJavascriptCallback(base::Value(callback_id_), response);
  callback_id_.clear();
}

void CookiesViewHandler::SendChildren(const CookieTreeNode* parent) {
  std::unique_ptr<base::ListValue> children(new base::ListValue);
  // Passing false for |include_quota_nodes| since they don't reflect reality
  // until bug http://crbug.com/642955 is fixed and local/session storage is
  // counted against the total.
  model_util_->GetChildNodeList(parent, /*start=*/0, parent->child_count(),
      /*include_quota_nodes=*/false, children.get());

  base::DictionaryValue args;
  if (parent == cookies_tree_model_->GetRoot())
    args.Set(kId, base::MakeUnique<base::Value>());
  else
    args.SetString(kId, model_util_->GetTreeNodeId(parent));
  args.Set(kChildren, std::move(children));

  ResolveJavascriptCallback(base::Value(callback_id_), args);
  callback_id_.clear();
}

void CookiesViewHandler::SendCookieDetails(const CookieTreeNode* parent) {
  std::unique_ptr<base::ListValue> children(new base::ListValue);
  // Passing false for |include_quota_nodes| since they don't reflect reality
  // until bug http://crbug.com/642955 is fixed and local/session storage is
  // counted against the total.
  model_util_->GetChildNodeDetails(parent, /*start=*/0, parent->child_count(),
                                   /*include_quota_nodes=*/false,
                                   children.get());

  base::DictionaryValue args;
  if (parent == cookies_tree_model_->GetRoot())
    args.Set(kId, base::MakeUnique<base::Value>());
  else
    args.SetString(kId, model_util_->GetTreeNodeId(parent));
  args.Set(kChildren, std::move(children));

  ResolveJavascriptCallback(base::Value(callback_id_), args);
  callback_id_.clear();
}

}  // namespace settings
