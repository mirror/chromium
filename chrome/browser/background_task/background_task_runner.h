// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK

#ifndef CHROME_BROWSER_BACKGROUND_TASK_BACKGROUND_TASK_RUNNER_
#define CHROME_BROWSER_BACKGROUND_TASK_BACKGROUND_TASK_RUNNER_

#include "base/task.h"
#include "chrome/browser/render_view_host_delegate.h"
#include "webkit/glue/webpreferences.h"

struct BackgroundTask;
class BackgroundTaskManager;
class SiteInstance;

// Defines a light-weight wrapper around a RenderViewHost responsible for
// running HTML and JS of a background task.
class BackgroundTaskRunner : public RenderViewHostDelegate {
 public:
  BackgroundTaskRunner(BackgroundTaskManager* background_task_manager,
                       BackgroundTask* background_task);
  virtual ~BackgroundTaskRunner() {}

  // Helper to find the background task that originated the given request.
  static BackgroundTask* GetBackgroundTaskByID(int render_process_id,
                                               int render_view_id);

  // Starts the background task.
  bool Start();

  // Shuts down the background task.
  void Shutdown();

  // RenderViewHostDelegate implementations.
  virtual RenderViewHostDelegateType GetDelegateType() const {
    return RenderViewHostDelegate::BACKGROUND_TASK_DELEGATE;
  }
  virtual WebPreferences GetWebkitPrefs();
  virtual Profile* GetProfile() const;
  virtual void RendererReady(RenderViewHost* render_view_host);
  virtual void RendererGone(RenderViewHost* render_view_host);
  virtual void UpdateTitle(RenderViewHost* render_view_host,
                           int32 page_id,
                           const std::wstring& title);
  virtual void RunJavaScriptMessage(const std::wstring& message,
                                    const std::wstring& prompt,
                                    const int flags,
                                    IPC::Message* reply_msg);

  // Accessors.
  SiteInstance* task_instance() const { return task_instance_; }
  RenderViewHost* render_view_host() const { return render_view_host_; }
  const std::wstring& title() const { return title_; }

 private:
  // Helper functions for sending notifications.
  void NotifyConnected();
  void NotifyDisconnected();

  // Does the delayed shutdown.
  void DelayedShutdown();

  // The background task manager.
  BackgroundTaskManager* background_task_manager_;

  // The background task.
  BackgroundTask* background_task_;

  // The site instance.
  SiteInstance* task_instance_;

  // The host HTML runner.
  RenderViewHost* render_view_host_;

  // Indicates whether we should notify about disconnection of this background
  // task. This is used to ensure disconnection notifications only happen if
  // a connection notification has happened and that they happen only once.
  bool notify_disconnection_;

  // The title from the task page. This is used in the task manager display.
  std::wstring title_;

  // The following factory is used to call methods at a later time.
  ScopedRunnableMethodFactory<BackgroundTaskRunner> method_factory_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundTaskRunner);
};

#endif  // CHROME_BROWSER_BACKGROUND_TASK_MANAGER_H_

#endif  // CHROME_BROWSER_BACKGROUND_TASK_BACKGROUND_TASK_RUNNER_
