// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/in_process_webkit/indexed_db_context.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "content/browser/browser_thread.h"
#include "content/browser/in_process_webkit/indexed_db_quota_client.h"
#include "content/browser/in_process_webkit/webkit_context.h"
#include "googleurl/src/gurl.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCString.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebIDBDatabase.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebIDBFactory.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebSecurityOrigin.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebString.h"
#include "webkit/glue/webkit_glue.h"
#include "webkit/quota/quota_manager.h"
#include "webkit/quota/special_storage_policy.h"

using WebKit::WebIDBDatabase;
using WebKit::WebIDBFactory;
using WebKit::WebSecurityOrigin;

namespace {

void ClearLocalState(
    const FilePath& indexeddb_path,
    scoped_refptr<quota::SpecialStoragePolicy> special_storage_policy) {
  file_util::FileEnumerator file_enumerator(
      indexeddb_path, false, file_util::FileEnumerator::FILES);
  // TODO(pastarmovj): We might need to consider exchanging this loop for
  // something more efficient in the future.
  for (FilePath file_path = file_enumerator.Next(); !file_path.empty();
       file_path = file_enumerator.Next()) {
    if (file_path.Extension() != IndexedDBContext::kIndexedDBExtension)
      continue;
    WebSecurityOrigin origin =
        WebSecurityOrigin::createFromDatabaseIdentifier(
            webkit_glue::FilePathToWebString(file_path.BaseName()));
    if (special_storage_policy->IsStorageProtected(GURL(origin.toString())))
      continue;
    file_util::Delete(file_path, false);
  }
}

}  // namespace

const FilePath::CharType IndexedDBContext::kIndexedDBDirectory[] =
    FILE_PATH_LITERAL("IndexedDB");

const FilePath::CharType IndexedDBContext::kIndexedDBExtension[] =
    FILE_PATH_LITERAL(".leveldb");

IndexedDBContext::IndexedDBContext(
    WebKitContext* webkit_context,
    quota::SpecialStoragePolicy* special_storage_policy,
    quota::QuotaManagerProxy* quota_manager_proxy,
    base::MessageLoopProxy* webkit_thread_loop)
    : clear_local_state_on_exit_(false),
      special_storage_policy_(special_storage_policy) {
  data_path_ = webkit_context->data_path().Append(kIndexedDBDirectory);
  if (quota_manager_proxy) {
    quota_manager_proxy->RegisterClient(
        new IndexedDBQuotaClient(webkit_thread_loop, this));
  }
}

IndexedDBContext::~IndexedDBContext() {
  WebKit::WebIDBFactory* factory = idb_factory_.release();
  if (factory)
    BrowserThread::DeleteSoon(BrowserThread::WEBKIT, FROM_HERE, factory);

  if (clear_local_state_on_exit_) {
    // No WEBKIT thread here means we are running in a unit test where no clean
    // up is needed.
    BrowserThread::PostTask(BrowserThread::WEBKIT, FROM_HERE,
        NewRunnableFunction(&ClearLocalState, data_path_,
                            special_storage_policy_));
  }
}

WebIDBFactory* IndexedDBContext::GetIDBFactory() {
  if (!idb_factory_.get())
    idb_factory_.reset(WebIDBFactory::create());
  DCHECK(idb_factory_.get());
  return idb_factory_.get();
}

FilePath IndexedDBContext::GetIndexedDBFilePath(
    const string16& origin_id) const {
  FilePath::StringType id =
      webkit_glue::WebStringToFilePathString(origin_id).append(
          FILE_PATH_LITERAL(".indexeddb"));
  return data_path_.Append(id.append(kIndexedDBExtension));
}

void IndexedDBContext::DeleteIndexedDBFile(const FilePath& file_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::WEBKIT));
  // TODO(pastarmovj): Close all database connections that use that file.
  file_util::Delete(file_path, false);
}

void IndexedDBContext::DeleteIndexedDBForOrigin(const string16& origin_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::WEBKIT));
  // TODO(pastarmovj): Remove this check once we are safe to delete any time.
  FilePath idb_file = GetIndexedDBFilePath(origin_id);
  DCHECK_EQ(idb_file.BaseName().value().substr(0, strlen("chrome-extension")),
            FILE_PATH_LITERAL("chrome-extension"));
  DeleteIndexedDBFile(GetIndexedDBFilePath(origin_id));
}

bool IndexedDBContext::IsUnlimitedStorageGranted(
    const GURL& origin) const {
  return special_storage_policy_->IsStorageUnlimited(origin);
}

// TODO(dgrogan): Merge this code with the similar loop in
// BrowsingDataIndexedDBHelperImpl::FetchIndexedDBInfoInWebKitThread.
void IndexedDBContext::GetAllOriginIdentifiers(
    std::vector<string16>* origin_ids) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::WEBKIT));
  file_util::FileEnumerator file_enumerator(data_path_,
      false, file_util::FileEnumerator::DIRECTORIES);
  for (FilePath file_path = file_enumerator.Next(); !file_path.empty();
       file_path = file_enumerator.Next()) {
    if (file_path.Extension() == IndexedDBContext::kIndexedDBExtension) {
      WebKit::WebString origin_id_webstring =
          webkit_glue::FilePathToWebString(file_path.BaseName());
      origin_ids->push_back(origin_id_webstring);
    }
  }
}
