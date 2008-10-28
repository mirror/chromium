// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#include "chrome/browser/background_task/background_task_runner.h"

#include "chrome/browser/background_task/background_task_manager.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/render_view_host.h"
#include "chrome/browser/render_widget_host_view_win.h"
#include "chrome/browser/site_instance.h"
#include "chrome/common/notification_types.h"

// static
BackgroundTask* BackgroundTaskRunner::GetBackgroundTaskByID(
    int render_process_id, int render_view_id) {
  RenderViewHost* render_view_host =
      RenderViewHost::FromID(render_process_id, render_view_id);
  if (!render_view_host) {
    return NULL;
  }

  return static_cast<BackgroundTaskRunner*>(render_view_host->delegate())
      ->background_task_;
}

BackgroundTaskRunner::BackgroundTaskRunner(
    BackgroundTaskManager* background_task_manager,
    BackgroundTask* background_task)
    : background_task_manager_(background_task_manager),
      background_task_(background_task),
      render_view_host_(NULL),
      notify_disconnection_(false),
      method_factory_(this) {
  DCHECK(background_task_ && !background_task_->runner);
  background_task_->runner = this;
}

void BackgroundTaskRunner::Shutdown() {
  NotifyDisconnected();

  // Need to do this asynchronously as it will close the RenderViewHost, which
  // is currently on the call stack above us.
  MessageLoop::current()->PostTask(FROM_HERE,
      method_factory_.NewRunnableMethod(
          &BackgroundTaskRunner::DelayedShutdown));
}

void BackgroundTaskRunner::DelayedShutdown() {
  if (render_view_host_) {
    render_view_host_->Shutdown();
    render_view_host_ = NULL;
  }

  background_task_->runner = NULL;
  delete this;
}

bool BackgroundTaskRunner::Start() {
  SiteInstance* task_instance =
      SiteInstance::CreateSiteInstance(background_task_manager_->profile());
  task_instance->SetSite(background_task_->url.GetOrigin());
  
  render_view_host_ = new RenderViewHost(task_instance,
                                         this,
                                         MSG_ROUTING_NONE,
                                         NULL);

  // TODO (jianli): remove HWND since we don't render background task.
  RenderWidgetHostViewWin* view =
      new RenderWidgetHostViewWin(render_view_host_);
  render_view_host_->set_view(view);
  HWND root_hwnd = NULL;
  if (BrowserList::GetLastActive()) {
    root_hwnd = BrowserList::GetLastActive()->GetTopLevelHWND();
  }
  if (!root_hwnd) {
    NOTREACHED();
    return false;
  }
  view->Create(root_hwnd);

  render_view_host_->CreateRenderView();
  render_view_host_->NavigateToURL(background_task_->url);

  return true;
}

WebPreferences BackgroundTaskRunner::GetWebkitPrefs() {
  return WebPreferences();
}

Profile* BackgroundTaskRunner::GetProfile() const {
  return background_task_manager_->profile();
}

void BackgroundTaskRunner::RendererReady(RenderViewHost* render_view_host) {
  NotifyConnected();
}

void BackgroundTaskRunner::RendererGone(RenderViewHost* render_view_host) {
  Shutdown();
}

void BackgroundTaskRunner::UpdateTitle(RenderViewHost* render_view_host,
                                       int32 page_id,
                                       const std::wstring& title) {
  title_ = title;
}

void BackgroundTaskRunner::NotifyConnected() {
  notify_disconnection_ = true;
  NotificationService::current()->
      Notify(NOTIFY_BACKGROUND_TASK_CONNECTED,
             Source<BackgroundTask>(background_task_),
             NotificationService::NoDetails());
}

void BackgroundTaskRunner::NotifyDisconnected() {
  if (!notify_disconnection_) {
    return;
  }
  notify_disconnection_ = false;
  NotificationService::current()->
      Notify(NOTIFY_BACKGROUND_TASK_DISCONNECTED,
             Source<BackgroundTask>(background_task_),
             NotificationService::NoDetails());
}

#endif  // ENABLE_BACKGROUND_TASK
