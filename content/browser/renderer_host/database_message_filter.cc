// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/database_message_filter.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "content/browser/bad_message.h"
#include "content/common/database_messages.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/bind_interface_helpers.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/result_codes.h"
#include "storage/browser/database/database_util.h"
#include "storage/browser/database/vfs_backend.h"
#include "storage/browser/quota/quota_manager.h"
#include "storage/browser/quota/quota_manager_proxy.h"
#include "storage/common/database/database_identifier.h"
#include "third_party/sqlite/sqlite3.h"
#include "url/origin.h"

#if defined(OS_POSIX)
#include "base/file_descriptor_posix.h"
#endif

using storage::QuotaManager;
using storage::QuotaStatusCode;
using storage::DatabaseTracker;
using storage::DatabaseUtil;
using storage::VfsBackend;

namespace content {
namespace {


bool IsOriginValid(const url::Origin& origin) {
  return !origin.unique();
}

}  // namespace

DatabaseMessageFilter::DatabaseMessageFilter(
    int process_id,
    storage::DatabaseTracker* db_tracker)
    : BrowserMessageFilter(DatabaseMsgStart),
      process_id_(process_id),
      db_tracker_(db_tracker),
      observer_added_(false) {
  DCHECK(db_tracker_.get());
}

void DatabaseMessageFilter::OnChannelClosing() {
  db_tracker_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DatabaseMessageFilter::CloseOnDatabaseTrackerTask, this));
}

void DatabaseMessageFilter::AddObserver() {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());
  db_tracker_->AddObserver(this);
}

void DatabaseMessageFilter::RemoveObserver() {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());
  db_tracker_->RemoveObserver(this);

  // If the renderer process died without closing all databases,
  // then we need to manually close those connections
  db_tracker_->CloseDatabases(database_connections_);
  database_connections_.RemoveAllConnections();
}

base::TaskRunner* DatabaseMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  // GetSpaceAvailable talks to the quota manager on IO thread, so avoid
  // multiple hops.
  if (message.type() == DatabaseHostMsg_GetSpaceAvailable::ID)
    return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO).get();

  if (IPC_MESSAGE_CLASS(message) == DatabaseMsgStart)
    return db_tracker_->task_runner();

  return nullptr;
}

bool DatabaseMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(DatabaseMessageFilter, message)
    IPC_MESSAGE_HANDLER(DatabaseHostMsg_GetFileSize, OnDatabaseGetFileSize)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(DatabaseHostMsg_GetSpaceAvailable,
                                    OnDatabaseGetSpaceAvailable)
    IPC_MESSAGE_HANDLER(DatabaseHostMsg_SetFileSize, OnDatabaseSetFileSize)
    IPC_MESSAGE_HANDLER(DatabaseHostMsg_Opened, OnDatabaseOpened)
    IPC_MESSAGE_HANDLER(DatabaseHostMsg_Modified, OnDatabaseModified)
    IPC_MESSAGE_HANDLER(DatabaseHostMsg_Closed, OnDatabaseClosed)
    IPC_MESSAGE_HANDLER(DatabaseHostMsg_HandleSqliteError, OnHandleSqliteError)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

DatabaseMessageFilter::~DatabaseMessageFilter() {
}

void DatabaseMessageFilter::OnDatabaseGetFileSize(
    const base::string16& vfs_file_name,
    int64_t* size) {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());
  *size = 0;
  base::FilePath db_file =
      DatabaseUtil::GetFullFilePathForVfsFile(db_tracker_.get(), vfs_file_name);
  if (!db_file.empty())
    *size = VfsBackend::GetFileSize(db_file);
}

void DatabaseMessageFilter::OnDatabaseGetSpaceAvailable(
    const url::Origin& origin,
    IPC::Message* reply_msg) {
  // Note the special case in OverrideTaskRunnerForMessage - since this
  // interacts directy with the QuotaManager this handler is run on the IO
  // thread to avoid unnecessary thread hops.
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(db_tracker_->quota_manager_proxy());

  if (!IsOriginValid(origin)) {
    bad_message::ReceivedBadMessage(
        this, bad_message::DBMF_INVALID_ORIGIN_ON_GET_SPACE);
    return;
  }

  QuotaManager* quota_manager =
      db_tracker_->quota_manager_proxy()->quota_manager();
  if (!quota_manager) {
    NOTREACHED();  // The system is shutting down, messages are unexpected.
    DatabaseHostMsg_GetSpaceAvailable::WriteReplyParams(
        reply_msg, static_cast<int64_t>(0));
    Send(reply_msg);
    return;
  }

  // crbug.com/349708
  TRACE_EVENT0("io", "DatabaseMessageFilter::OnDatabaseGetSpaceAvailable");

  quota_manager->GetUsageAndQuota(
      origin.GetURL(), storage::kStorageTypeTemporary,
      base::Bind(&DatabaseMessageFilter::OnDatabaseDidGetUsageAndQuota, this,
                 reply_msg));
}

void DatabaseMessageFilter::OnDatabaseDidGetUsageAndQuota(
    IPC::Message* reply_msg,
    storage::QuotaStatusCode status,
    int64_t usage,
    int64_t quota) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int64_t available = 0;
  if ((status == storage::kQuotaStatusOk) && (usage < quota))
    available = quota - usage;
  DatabaseHostMsg_GetSpaceAvailable::WriteReplyParams(reply_msg, available);
  Send(reply_msg);
}

void DatabaseMessageFilter::OnDatabaseSetFileSize(
    const base::string16& vfs_file_name,
    int64_t size,
    bool* success) {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());
  *success = false;
  base::FilePath db_file =
      DatabaseUtil::GetFullFilePathForVfsFile(db_tracker_.get(), vfs_file_name);
  if (!db_file.empty())
    *success = VfsBackend::SetFileSize(db_file, size);
}

void DatabaseMessageFilter::OnDatabaseOpened(
    const url::Origin& origin,
    const base::string16& database_name,
    const base::string16& description,
    int64_t estimated_size) {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());

  if (!observer_added_) {
    observer_added_ = true;
    AddObserver();
  }

  if (!IsOriginValid(origin)) {
    bad_message::ReceivedBadMessage(this,
                                    bad_message::DBMF_INVALID_ORIGIN_ON_OPEN);
    return;
  }

  GURL origin_url(origin.Serialize());
  UMA_HISTOGRAM_BOOLEAN("websql.OpenDatabase", IsOriginSecure(origin_url));

  int64_t database_size = 0;
  std::string origin_identifier(storage::GetIdentifierFromOrigin(origin_url));
  db_tracker_->DatabaseOpened(origin_identifier, database_name, description,
                              estimated_size, &database_size);

  database_connections_.AddConnection(origin_identifier, database_name);

  GetWebDatabase().UpdateSize(origin, database_name, database_size);
}

void DatabaseMessageFilter::OnDatabaseModified(
    const url::Origin& origin,
    const base::string16& database_name) {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());

  if (!IsOriginValid(origin)) {
    bad_message::ReceivedBadMessage(
        this, bad_message::DBMF_INVALID_ORIGIN_ON_MODIFIED);
    return;
  }

  std::string origin_identifier(
      storage::GetIdentifierFromOrigin(origin.GetURL()));
  if (!database_connections_.IsDatabaseOpened(origin_identifier,
                                              database_name)) {
    bad_message::ReceivedBadMessage(this,
                                    bad_message::DBMF_DB_NOT_OPEN_ON_MODIFY);
    return;
  }

  db_tracker_->DatabaseModified(origin_identifier, database_name);
}

void DatabaseMessageFilter::OnDatabaseClosed(
    const url::Origin& origin,
    const base::string16& database_name) {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());

  if (!IsOriginValid(origin)) {
    bad_message::ReceivedBadMessage(this,
                                    bad_message::DBMF_INVALID_ORIGIN_ON_CLOSED);
    return;
  }

  std::string origin_identifier(
      storage::GetIdentifierFromOrigin(origin.GetURL()));
  if (!database_connections_.IsDatabaseOpened(
          origin_identifier, database_name)) {
    bad_message::ReceivedBadMessage(this,
                                    bad_message::DBMF_DB_NOT_OPEN_ON_CLOSE);
    return;
  }

  database_connections_.RemoveConnection(origin_identifier, database_name);
  db_tracker_->DatabaseClosed(origin_identifier, database_name);
}

void DatabaseMessageFilter::OnHandleSqliteError(
    const url::Origin& origin,
    const base::string16& database_name,
    int error) {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());
  if (!IsOriginValid(origin)) {
    bad_message::ReceivedBadMessage(
        this, bad_message::DBMF_INVALID_ORIGIN_ON_SQLITE_ERROR);
    return;
  }
  db_tracker_->HandleSqliteError(
      storage::GetIdentifierFromOrigin(origin.GetURL()), database_name, error);
}

void DatabaseMessageFilter::OnDatabaseSizeChanged(
    const std::string& origin_identifier,
    const base::string16& database_name,
    int64_t database_size) {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());
  if (!database_connections_.IsOriginUsed(origin_identifier)) {
    return;
  }

  GetWebDatabase().UpdateSize(
      url::Origin(storage::GetOriginFromIdentifier(origin_identifier)),
      database_name, database_size);
}

void DatabaseMessageFilter::OnDatabaseScheduledForDeletion(
    const std::string& origin_identifier,
    const base::string16& database_name) {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());

  GetWebDatabase().CloseImmediately(
      url::Origin(storage::GetOriginFromIdentifier(origin_identifier)),
      database_name);
}

void DatabaseMessageFilter::CloseOnDatabaseTrackerTask() {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());
  if (observer_added_) {
    observer_added_ = false;
    RemoveObserver();
  }
  database_provider_.reset();
}

content::mojom::WebDatabase& DatabaseMessageFilter::GetWebDatabase() {
  DCHECK(db_tracker_->task_runner()->RunsTasksInCurrentSequence());
  if (!database_provider_) {
    // The interface binding needs to occur on the UI thread, as we can
    // only call RenderProcessHost::FromID() on the UI thread.
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(
            [](int process_id, content::mojom::WebDatabaseRequest request) {
              RenderProcessHost* host = RenderProcessHost::FromID(process_id);
              if (host) {
                content::BindInterface(host, std::move(request));
              }
            },
            process_id_, mojo::MakeRequest(&database_provider_)));
  }
  return *database_provider_;
}

}  // namespace content
