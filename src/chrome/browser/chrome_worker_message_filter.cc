// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_worker_message_filter.h"

#include "base/bind.h"
#include "chrome/browser/content_settings/cookie_settings.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "content/browser/resource_context.h"
#include "content/browser/worker_host/worker_process_host.h"
#include "content/common/worker_messages.h"

using content::BrowserThread;

ChromeWorkerMessageFilter::ChromeWorkerMessageFilter(WorkerProcessHost* process)
    : process_(process) {
  ProfileIOData* io_data = reinterpret_cast<ProfileIOData*>(
      process->resource_context()->GetUserData(NULL));
  cookie_settings_ = io_data->GetCookieSettings();
}

ChromeWorkerMessageFilter::~ChromeWorkerMessageFilter() {
}

bool ChromeWorkerMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ChromeWorkerMessageFilter, message)
    IPC_MESSAGE_HANDLER(WorkerProcessHostMsg_AllowDatabase, OnAllowDatabase)
    IPC_MESSAGE_HANDLER(WorkerProcessHostMsg_AllowFileSystem, OnAllowFileSystem)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

bool ChromeWorkerMessageFilter::Send(IPC::Message* message) {
  return process_->Send(message);
}

void ChromeWorkerMessageFilter::OnAllowDatabase(int worker_route_id,
                                                const GURL& url,
                                                const string16& name,
                                                const string16& display_name,
                                                unsigned long estimated_size,
                                                bool* result) {
  *result = cookie_settings_->IsSettingCookieAllowed(url, url);

  // Record access to database for potential display in UI: Find the worker
  // instance and forward the message to all attached documents.
  WorkerProcessHost::Instances::const_iterator i;
  for (i = process_->instances().begin(); i != process_->instances().end();
       ++i) {
    if (i->worker_route_id() != worker_route_id)
      continue;
    const WorkerDocumentSet::DocumentInfoSet& documents =
        i->worker_document_set()->documents();
    for (WorkerDocumentSet::DocumentInfoSet::const_iterator doc =
         documents.begin(); doc != documents.end(); ++doc) {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(
              &TabSpecificContentSettings::WebDatabaseAccessed,
              doc->render_process_id(), doc->render_view_id(),
              url, name, display_name, !*result));
    }
    break;
  }
}

void ChromeWorkerMessageFilter::OnAllowFileSystem(int worker_route_id,
                                                  const GURL& url,
                                                  bool* result) {
  *result = cookie_settings_->IsSettingCookieAllowed(url, url);

  // Record access to file system for potential display in UI: Find the worker
  // instance and forward the message to all attached documents.
  WorkerProcessHost::Instances::const_iterator i;
  for (i = process_->instances().begin(); i != process_->instances().end();
       ++i) {
    if (i->worker_route_id() != worker_route_id)
      continue;
    const WorkerDocumentSet::DocumentInfoSet& documents =
        i->worker_document_set()->documents();
    for (WorkerDocumentSet::DocumentInfoSet::const_iterator doc =
         documents.begin(); doc != documents.end(); ++doc) {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(
              &TabSpecificContentSettings::FileSystemAccessed,
              doc->render_process_id(), doc->render_view_id(), url, !*result));
    }
    break;
  }
}
