// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/logging_win.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/process/process.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/template_util.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "chrome/chrome_watcher/chrome_watcher_main_api.h"
#include "components/browser_watcher/endsession_watcher_window_win.h"
#include "components/browser_watcher/exit_code_watcher_win.h"
#include "components/browser_watcher/exit_funnel_win.h"

namespace {

// Use the same log facility as Chrome for convenience.
// {7FE69228-633E-4f06-80C1-527FEA23E3A7}
const GUID kChromeWatcherTraceProviderName = {
    0x7fe69228, 0x633e, 0x4f06,
        { 0x80, 0xc1, 0x52, 0x7f, 0xea, 0x23, 0xe3, 0xa7 } };

// Takes care of monitoring a browser. This class watches for a browser's exit
// code, as well as listening for WM_ENDSESSION messages. Events are recorded
// in an exit funnel, for reporting the next time Chrome runs.
class BrowserMonitor {
 public:
  BrowserMonitor(base::RunLoop* run_loop, const base::char16* registry_path);
  ~BrowserMonitor();

  // Starts the monitor, returns true on success.
  bool StartWatching(const base::char16* registry_path,
                     base::Process process);

 private:
  // Called from EndSessionWatcherWindow on a WM_ENDSESSION message.
  void OnEndSession(LPARAM lparam);

  // Blocking function that runs on |background_thread_|.
  void Watch();

  // Posted to main thread from Watch when browser exits.
  void BrowserExited();

  // True if BrowserExited has run.
  bool browser_exited_;

  // The funnel used to record events for this browser.
  browser_watcher::ExitFunnel exit_funnel_;

  browser_watcher::ExitCodeWatcher exit_code_watcher_;
  browser_watcher::EndSessionWatcherWindow end_session_watcher_window_;

  // The thread that runs Watch().
  base::Thread background_thread_;

  // The run loop for the main thread and its task runner.
  base::RunLoop* run_loop_;
  scoped_refptr<base::SequencedTaskRunner> main_thread_;

  DISALLOW_COPY_AND_ASSIGN(BrowserMonitor);
};

BrowserMonitor::BrowserMonitor(base::RunLoop* run_loop,
                               const base::char16* registry_path) :
    browser_exited_(false),
    exit_code_watcher_(registry_path),
    end_session_watcher_window_(
        base::Bind(&BrowserMonitor::OnEndSession, base::Unretained(this))),
    background_thread_("BrowserWatcherThread"),
    run_loop_(run_loop),
    main_thread_(base::MessageLoopProxy::current()) {
}

BrowserMonitor::~BrowserMonitor() {
}

bool BrowserMonitor::StartWatching(const base::char16* registry_path,
                                   base::Process process) {
  if (!exit_code_watcher_.Initialize(process.Pass()))
    return false;

  if (!exit_funnel_.Init(registry_path,
                        exit_code_watcher_.process().Handle())) {
    return false;
  }

  if (!background_thread_.StartWithOptions(
        base::Thread::Options(base::MessageLoop::TYPE_IO, 0))) {
    return false;
  }

  if (!background_thread_.task_runner()->PostTask(FROM_HERE,
        base::Bind(&BrowserMonitor::Watch, base::Unretained(this)))) {
    background_thread_.Stop();
    return false;
  }

  return true;
}

void BrowserMonitor::OnEndSession(LPARAM lparam) {
  DCHECK_EQ(main_thread_, base::MessageLoopProxy::current());

  exit_funnel_.RecordEvent(L"WatcherLogoff");
  if (lparam & ENDSESSION_CLOSEAPP)
    exit_funnel_.RecordEvent(L"ES_CloseApp");
  if (lparam & ENDSESSION_CRITICAL)
    exit_funnel_.RecordEvent(L"ES_Critical");
  if (lparam & ENDSESSION_LOGOFF)
    exit_funnel_.RecordEvent(L"ES_Logoff");
  const LPARAM kKnownBits =
      ENDSESSION_CLOSEAPP | ENDSESSION_CRITICAL | ENDSESSION_LOGOFF;
  if (lparam & ~kKnownBits)
    exit_funnel_.RecordEvent(L"ES_Other");

  // Belt-and-suspenders; make sure our message loop exits ASAP.
  if (browser_exited_)
    run_loop_->Quit();
}

void BrowserMonitor::Watch() {
  // This needs to run on an IO thread.
  DCHECK_NE(main_thread_, base::MessageLoopProxy::current());

  exit_code_watcher_.WaitForExit();
  exit_funnel_.RecordEvent(L"BrowserExit");

  main_thread_->PostTask(FROM_HERE,
      base::Bind(&BrowserMonitor::BrowserExited, base::Unretained(this)));
}

void BrowserMonitor::BrowserExited() {
  // This runs in the main thread.
  DCHECK_EQ(main_thread_, base::MessageLoopProxy::current());

  // Note that the browser has exited.
  browser_exited_ = true;

  // Our background thread has served it's purpose.
  background_thread_.Stop();

  const int exit_code = exit_code_watcher_.exit_code();
  if (exit_code >= 0 && exit_code <= 28) {
    // The browser exited with a well-known exit code, quit this process
    // immediately.
    run_loop_->Quit();
  } else {
    // The browser exited abnormally, wait around for a little bit to see
    // whether this instance will get a logoff message.
    main_thread_->PostDelayedTask(FROM_HERE,
                                  run_loop_->QuitClosure(),
                                  base::TimeDelta::FromSeconds(30));
  }
}

}  // namespace

// The main entry point to the watcher, declared as extern "C" to avoid name
// mangling.
extern "C" int WatcherMain(const base::char16* registry_path,
                           HANDLE process_handle) {
  base::Process process(process_handle);

  // The exit manager is in charge of calling the dtors of singletons.
  base::AtExitManager exit_manager;
  // Initialize the commandline singleton from the environment.
  base::CommandLine::Init(0, nullptr);

  logging::LogEventProvider::Initialize(kChromeWatcherTraceProviderName);

  // Arrange to be shut down as late as possible, as we want to outlive
  // chrome.exe in order to report its exit status.
  ::SetProcessShutdownParameters(0x100, SHUTDOWN_NORETRY);

  // Run a UI message loop on the main thread.
  base::MessageLoop msg_loop(base::MessageLoop::TYPE_UI);
  msg_loop.set_thread_name("WatcherMainThread");

  base::RunLoop run_loop;
  BrowserMonitor monitor(&run_loop, registry_path);
  if (!monitor.StartWatching(registry_path, process.Pass()))
    return 1;

  run_loop.Run();

  // Wind logging down.
  logging::LogEventProvider::Uninitialize();

  return 0;
}

static_assert(
    base::is_same<decltype(&WatcherMain), ChromeWatcherMainFunction>::value,
    "WatcherMain() has wrong type");
